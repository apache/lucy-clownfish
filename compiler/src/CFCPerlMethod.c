/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define CFC_NEED_PERLSUB_STRUCT_DEF 1
#include "CFCPerlSub.h"
#include "CFCPerlMethod.h"
#include "CFCUtil.h"
#include "CFCClass.h"
#include "CFCFunction.h"
#include "CFCMethod.h"
#include "CFCSymbol.h"
#include "CFCType.h"
#include "CFCParcel.h"
#include "CFCParamList.h"
#include "CFCPerlTypeMap.h"
#include "CFCVariable.h"

struct CFCPerlMethod {
    CFCPerlSub  sub;
    CFCMethod  *method;
};

// Return the main chunk of the code for the xsub.
static char*
S_xsub_body(CFCPerlMethod *self);

// Create an assignment statement for extracting $self from the Perl stack.
static char*
S_self_assign_statement(CFCPerlMethod *self, CFCType *type);

// Return code for an xsub which uses labeled params.
static char*
S_xsub_def_labeled_params(CFCPerlMethod *self);

// Return code for an xsub which uses positional args.
static char*
S_xsub_def_positional_args(CFCPerlMethod *self);

/* Generate code which converts C types to Perl types and pushes arguments
 * onto the Perl stack.
 */
static char*
S_callback_start(CFCMethod *method);

/* Adapt the refcounts of parameters and return types.
 */
static char*
S_callback_refcount_mods(CFCMethod *method);

/* Return a function which throws a runtime error indicating which variable
 * couldn't be mapped.  TODO: it would be better to resolve all these cases at
 * compile-time.
 */
static char*
S_invalid_callback_def(CFCMethod *method);

// Create a callback for a method which operates in a void context.
static char*
S_void_callback_def(CFCMethod *method, const char *callback_start,
                    const char *refcount_mods);

// Create a callback which returns a primitive type.
static char*
S_primitive_callback_def(CFCMethod *method, const char *callback_start,
                         const char *refcount_mods);

/* Create a callback which returns an object type -- either a generic object or
 * a string. */
static char*
S_obj_callback_def(CFCMethod *method, const char *callback_start,
                   const char *refcount_mods);

static const CFCMeta CFCPERLMETHOD_META = {
    "Clownfish::CFC::Binding::Perl::Method",
    sizeof(CFCPerlMethod),
    (CFCBase_destroy_t)CFCPerlMethod_destroy
};

CFCPerlMethod*
CFCPerlMethod_new(CFCMethod *method) {
    CFCPerlMethod *self
        = (CFCPerlMethod*)CFCBase_allocate(&CFCPERLMETHOD_META);
    return CFCPerlMethod_init(self, method);
}

CFCPerlMethod*
CFCPerlMethod_init(CFCPerlMethod *self, CFCMethod *method) {
    CFCParamList *param_list = CFCMethod_get_param_list(method);
    const char *class_name = CFCMethod_get_class_name(method);
    int use_labeled_params = CFCParamList_num_vars(param_list) > 2
                             ? 1 : 0;

    char *perl_name = CFCPerlMethod_perl_name(method);
    CFCPerlSub_init((CFCPerlSub*)self, param_list, class_name, perl_name,
                    use_labeled_params);
    self->method = (CFCMethod*)CFCBase_incref((CFCBase*)method);
    FREEMEM(perl_name);
    return self;
}

void
CFCPerlMethod_destroy(CFCPerlMethod *self) {
    CFCBase_decref((CFCBase*)self->method);
    CFCPerlSub_destroy((CFCPerlSub*)self);
}

int
CFCPerlMethod_can_be_bound(CFCMethod *method) {
    /*
     * Check for
     * - private methods
     * - methods with types which cannot be mapped automatically
     */
    return !CFCSymbol_private((CFCSymbol*)method)
           && CFCFunction_can_be_bound((CFCFunction*)method);
}

char*
CFCPerlMethod_perl_name(CFCMethod *method) {
    // See if the user wants the method to have a specific alias.
    const char *alias = CFCMethod_get_host_alias(method);
    if (alias) {
        return CFCUtil_strdup(alias);
    }

    // Derive Perl name by lowercasing.
    const char *name      = CFCMethod_get_macro_sym(method);
    char       *perl_name = CFCUtil_strdup(name);
    for (size_t i = 0; perl_name[i] != '\0'; i++) {
        perl_name[i] = tolower(perl_name[i]);
    }

    return perl_name;
}

char*
CFCPerlMethod_xsub_def(CFCPerlMethod *self) {
    if (self->sub.use_labeled_params) {
        return S_xsub_def_labeled_params(self);
    }
    else {
        return S_xsub_def_positional_args(self);
    }
}

static char*
S_xsub_body(CFCPerlMethod *self) {
    CFCMethod    *method     = self->method;
    CFCParamList *param_list = CFCMethod_get_param_list(method);
    CFCVariable **arg_vars   = CFCParamList_get_variables(param_list);
    char         *name_list  = CFCPerlSub_arg_name_list((CFCPerlSub*)self);
    char         *body       = CFCUtil_strdup("");

    CFCParcel *parcel = CFCMethod_get_parcel(method);
    const char *class_name = CFCMethod_get_class_name(method);
    CFCClass *klass = CFCClass_fetch_singleton(parcel, class_name);
    if (!klass) {
        CFCUtil_die("Can't find a CFCClass for '%s'", class_name);
    }

    // Extract the method function pointer.
    char *full_meth = CFCMethod_full_method_sym(method, klass);
    char *method_ptr
        = CFCUtil_sprintf("method = CFISH_METHOD_PTR(%s, %s);\n    ",
                          CFCClass_full_class_var(klass), full_meth);
    body = CFCUtil_cat(body, method_ptr, NULL);
    FREEMEM(full_meth);
    FREEMEM(method_ptr);

    // Compensate for functions which eat refcounts.
    for (int i = 0; arg_vars[i] != NULL; i++) {
        CFCVariable *var = arg_vars[i];
        CFCType     *type = CFCVariable_get_type(var);
        if (CFCType_is_object(type) && CFCType_decremented(type)) {
            body = CFCUtil_cat(body, "CFISH_INCREF(arg_",
                               CFCVariable_micro_sym(var), ");\n    ", NULL);
        }
    }

    if (CFCType_is_void(CFCMethod_get_return_type(method))) {
        // Invoke method in void context.
        body = CFCUtil_cat(body, "method(", name_list,
                           ");\n    XSRETURN(0);", NULL);
    }
    else {
        // Return a value for method invoked in a scalar context.
        CFCType *return_type = CFCMethod_get_return_type(method);
        char *assignment = CFCPerlTypeMap_to_perl(return_type, "retval");
        if (!assignment) {
            const char *type_str = CFCType_to_c(return_type);
            CFCUtil_die("Can't find typemap for '%s'", type_str);
        }
        body = CFCUtil_cat(body, "retval = method(", name_list,
                           ");\n    ST(0) = ", assignment, ";", NULL);
        if (CFCType_is_object(return_type)
            && CFCType_incremented(return_type)
           ) {
            body = CFCUtil_cat(body, "\n    CFISH_DECREF(retval);", NULL);
        }
        body = CFCUtil_cat(body, "\n    sv_2mortal( ST(0) );\n    XSRETURN(1);",
                           NULL);
        FREEMEM(assignment);
    }

    FREEMEM(name_list);

    return body;
}

// Create an assignment statement for extracting $self from the Perl stack.
static char*
S_self_assign_statement(CFCPerlMethod *self, CFCType *type) {
    (void)self; // unused
    const char *type_c = CFCType_to_c(type);
    if (!CFCType_is_object(type)) {
        CFCUtil_die("Not an object type: %s", type_c);
    }
    const char *class_var = CFCType_get_class_var(type);
    char pattern[] = "arg_self = (%s)XSBind_sv_to_cfish_obj(ST(0), %s, NULL);";
    char *statement = CFCUtil_sprintf(pattern, type_c, class_var);

    return statement;
}

static char*
S_xsub_def_labeled_params(CFCPerlMethod *self) {
    CFCMethod *method = self->method;
    const char *c_name = self->sub.c_name;
    CFCParamList *param_list = self->sub.param_list;
    CFCVariable **arg_vars   = CFCParamList_get_variables(param_list);
    CFCVariable *self_var    = arg_vars[0];
    CFCType     *self_type   = CFCVariable_get_type(self_var);
    CFCType     *return_type = CFCMethod_get_return_type(method);
    const char  *self_type_c    = CFCType_to_c(self_type);
    const char  *self_micro_sym = CFCVariable_micro_sym(self_var);
    char *arg_decls    = CFCPerlSub_arg_declarations((CFCPerlSub*)self);
    char *meth_type_c  = CFCMethod_full_typedef(method, NULL);
    char *self_assign  = S_self_assign_statement(self, self_type);
    char *allot_params = CFCPerlSub_build_allot_params((CFCPerlSub*)self);
    char *body         = S_xsub_body(self);

    char *retval_decl;
    if (CFCType_is_void(return_type)) {
        retval_decl = CFCUtil_strdup("");
    }
    else {
        const char *return_type_c = CFCType_to_c(return_type);
        retval_decl = CFCUtil_sprintf("    %s retval;\n", return_type_c);
    }

    char pattern[] =
        "XS(%s);\n"
        "XS(%s) {\n"
        "    dXSARGS;\n"
        "    %s arg_self;\n"
        "%s"
        "    %s method;\n"
        "    bool args_ok;\n"
        "%s"
        "\n"
        "    CFISH_UNUSED_VAR(cv);\n"
        "    if (items < 1) { CFISH_THROW(CFISH_ERR, \"Usage: %%s(%s, ...)\",  GvNAME(CvGV(cv))); }\n"
        "    SP -= items;\n"
        "\n"
        "    /* Extract vars from Perl stack. */\n"
        "    %s\n"
        "    %s\n"
        "\n"
        "    /* Execute */\n"
        "    %s\n"
        "}\n";
    char *xsub_def
        = CFCUtil_sprintf(pattern, c_name, c_name, self_type_c, arg_decls,
                          meth_type_c, retval_decl, self_micro_sym,
                          allot_params, self_assign, body);

    FREEMEM(arg_decls);
    FREEMEM(meth_type_c);
    FREEMEM(self_assign);
    FREEMEM(allot_params);
    FREEMEM(body);
    FREEMEM(retval_decl);
    return xsub_def;
}

static char*
S_xsub_def_positional_args(CFCPerlMethod *self) {
    CFCMethod *method = self->method;
    CFCParamList *param_list = CFCMethod_get_param_list(method);
    CFCVariable **arg_vars = CFCParamList_get_variables(param_list);
    CFCVariable *self_var    = arg_vars[0];
    CFCType     *self_type   = CFCVariable_get_type(self_var);
    CFCType     *return_type = CFCMethod_get_return_type(method);
    const char  *self_type_c = CFCType_to_c(self_type);
    const char **arg_inits = CFCParamList_get_initial_values(param_list);
    unsigned num_vars = (unsigned)CFCParamList_num_vars(param_list);
    char *arg_decls   = CFCPerlSub_arg_declarations((CFCPerlSub*)self);
    char *meth_type_c = CFCMethod_full_typedef(method, NULL);
    char *self_assign = S_self_assign_statement(self, self_type);
    char *body        = S_xsub_body(self);

    // Determine how many args are truly required and build an error check.
    unsigned min_required = 0;
    for (unsigned i = 0; i < num_vars; i++) {
        if (arg_inits[i] == NULL) {
            min_required = i + 1;
        }
    }
    char *xs_name_list = num_vars > 0
                         ? CFCUtil_strdup(CFCVariable_micro_sym(arg_vars[0]))
                         : CFCUtil_strdup("");
    for (unsigned i = 1; i < num_vars; i++) {
        const char *var_name = CFCVariable_micro_sym(arg_vars[i]);
        if (i < min_required) {
            xs_name_list = CFCUtil_cat(xs_name_list, ", ", var_name, NULL);
        }
        else {
            xs_name_list = CFCUtil_cat(xs_name_list, ", [", var_name, "]",
                                       NULL);
        }
    }
    const char num_args_pattern[] =
        "if (items %s %u) { CFISH_THROW(CFISH_ERR, \"Usage: %%s(%s)\", GvNAME(CvGV(cv))); }";
    char *num_args_check;
    if (min_required < num_vars) {
        num_args_check = CFCUtil_sprintf(num_args_pattern, "<", min_required,
                                         xs_name_list);
    }
    else {
        num_args_check = CFCUtil_sprintf(num_args_pattern, "!=", num_vars,
                                         xs_name_list);
    }

    // Var assignments.
    char *var_assignments = CFCUtil_strdup("");
    for (unsigned i = 1; i < num_vars; i++) {
        CFCVariable *var = arg_vars[i];
        const char  *val = arg_inits[i];
        const char  *var_name = CFCVariable_micro_sym(var);
        CFCType     *var_type = CFCVariable_get_type(var);
        const char  *type_c   = CFCType_to_c(var_type);

        char perl_stack_var[30];
        sprintf(perl_stack_var, "ST(%u)", i);
        char *conversion
            = CFCPerlTypeMap_from_perl(var_type, perl_stack_var);
        if (!conversion) {
            CFCUtil_die("Can't map type '%s'", type_c);
        }
        if (val) {
            char pattern[] =
                "\n    arg_%s = ( items >= %u && XSBind_sv_defined(ST(%u)) )"
                " ? %s : %s;";
            char *statement = CFCUtil_sprintf(pattern, var_name, i, i,
                                              conversion, val);
            var_assignments
                = CFCUtil_cat(var_assignments, statement, NULL);
            FREEMEM(statement);
        }
        else {
            var_assignments
                = CFCUtil_cat(var_assignments, "\n    arg_", var_name, " = ",
                              conversion, ";", NULL);
        }
        FREEMEM(conversion);
    }

    char *retval_decl;
    if (CFCType_is_void(return_type)) {
        retval_decl = CFCUtil_strdup("");
    }
    else {
        const char *return_type_c = CFCType_to_c(return_type);
        retval_decl = CFCUtil_sprintf("    %s retval;\n", return_type_c);
    }

    char pattern[] =
        "XS(%s);\n"
        "XS(%s) {\n"
        "    dXSARGS;\n"
        "    %s arg_self;\n"
        "%s"
        "    %s method;\n"
        "%s"
        "\n"
        "    CFISH_UNUSED_VAR(cv);\n"
        "    SP -= items;\n"
        "    %s;\n"
        "\n"
        "    /* Extract vars from Perl stack. */\n"
        "    %s\n"
        "    %s\n"
        "\n"
        "    /* Execute */\n"
        "    %s\n"
        "}\n";
    char *xsub
        = CFCUtil_sprintf(pattern, self->sub.c_name, self->sub.c_name,
                          self_type_c, arg_decls, meth_type_c, retval_decl,
                          num_args_check, self_assign, var_assignments, body);

    FREEMEM(num_args_check);
    FREEMEM(var_assignments);
    FREEMEM(arg_decls);
    FREEMEM(meth_type_c);
    FREEMEM(self_assign);
    FREEMEM(body);
    FREEMEM(retval_decl);
    return xsub;
}

char*
CFCPerlMethod_callback_dec(CFCMethod *method) {
    const char *override_sym = CFCMethod_full_override_sym(method);
    CFCType      *return_type  = CFCMethod_get_return_type(method);
    CFCParamList *param_list   = CFCMethod_get_param_list(method);
    const char   *ret_type_str = CFCType_to_c(return_type);
    const char   *params       = CFCParamList_to_c(param_list);
    char pattern[] =
        "%s\n"
        "%s(%s);\n";
    char *callback_dec
        = CFCUtil_sprintf(pattern, ret_type_str, override_sym, params);
    return callback_dec;
}

char*
CFCPerlMethod_callback_def(CFCMethod *method) {
    // Return a callback wrapper that throws an error if there are no
    // bindings for a method.
    if (!CFCPerlMethod_can_be_bound(method)) {
        return S_invalid_callback_def(method);
    }

    CFCType *return_type = CFCMethod_get_return_type(method);
    char *start = S_callback_start(method);
    char *callback_def = NULL;
    char *refcount_mods = S_callback_refcount_mods(method);

    if (CFCType_is_void(return_type)) {
        callback_def = S_void_callback_def(method, start, refcount_mods);
    }
    else if (CFCType_is_object(return_type)) {
        callback_def = S_obj_callback_def(method, start, refcount_mods);
    }
    else if (CFCType_is_integer(return_type)
             || CFCType_is_floating(return_type)
        ) {
        callback_def = S_primitive_callback_def(method, start, refcount_mods);
    }
    else {
        // Can't map return type.
        callback_def = S_invalid_callback_def(method);
    }

    FREEMEM(start);
    FREEMEM(refcount_mods);
    return callback_def;
}

static char*
S_callback_start(CFCMethod *method) {
    CFCParamList *param_list = CFCMethod_get_param_list(method);
    static const char pattern[] =
        "    dSP;\n"
        "    EXTEND(SP, %d);\n"
        "    ENTER;\n"
        "    SAVETMPS;\n"
        "    PUSHMARK(SP);\n"
        "    mPUSHs((SV*)CFISH_Obj_To_Host((cfish_Obj*)self));\n";
    int num_args = (int)CFCParamList_num_vars(param_list) - 1;
    int num_to_extend = num_args == 0 ? 1
                      : num_args == 1 ? 2
                      : 1 + (num_args * 2);
    char *params = CFCUtil_sprintf(pattern, num_to_extend);

    // Iterate over arguments, mapping them to Perl scalars.
    CFCVariable **arg_vars = CFCParamList_get_variables(param_list);
    for (int i = 1; arg_vars[i] != NULL; i++) {
        CFCVariable *var      = arg_vars[i];
        const char  *name     = CFCVariable_micro_sym(var);
        CFCType     *type     = CFCVariable_get_type(var);
        const char  *c_type   = CFCType_to_c(type);

        // Add labels when there are two or more parameters.
        if (num_args > 1) {
            char num_buf[20];
            sprintf(num_buf, "%d", (int)strlen(name));
            params = CFCUtil_cat(params, "    mPUSHp(\"", name, "\", ",
                                 num_buf, ");\n", NULL);
        }

        if (CFCType_is_string_type(type)) {
            // Convert Clownfish string type to UTF-8 Perl string scalars.
            params = CFCUtil_cat(params, "    mPUSHs(XSBind_str_to_sv(",
                                 "(cfish_String*)", name, "));\n", NULL);
        }
        else if (CFCType_is_object(type)) {
            // Wrap other Clownfish object types in Perl objects.
            params = CFCUtil_cat(params, "    mPUSHs(XSBind_cfish_to_perl(",
                                 "(cfish_Obj*)", name, "));\n", NULL);
        }
        else if (CFCType_is_integer(type)) {
            // Convert primitive integer types to IV Perl scalars.
            int width = (int)CFCType_get_width(type);
            if (width != 0 && width <= 4) {
                params = CFCUtil_cat(params, "    mPUSHi(",
                                     name, ");\n", NULL);
            }
            else {
                // If the Perl IV integer type is not wide enough, use
                // doubles.  This may be lossy if the value is above 2**52,
                // but practically speaking, it's important to handle numbers
                // between 2**32 and 2**52 cleanly.
                params = CFCUtil_cat(params,
                                     "    if (sizeof(IV) >= sizeof(", c_type,
                                     ")) { mPUSHi(", name, "); }\n",
                                     "    else { mPUSHn((double)", name,
                                     "); } // lossy \n", NULL);
            }
        }
        else if (CFCType_is_floating(type)) {
            // Convert primitive floating point types to NV Perl scalars.
            params = CFCUtil_cat(params, "    mPUSHn(",
                                 name, ");\n", NULL);
        }
        else {
            // Can't map variable type.
            const char *type_str = CFCType_to_c(type);
            CFCUtil_die("Can't map type '%s' to Perl", type_str);
        }
    }

    // Restore the Perl stack pointer.
    params = CFCUtil_cat(params, "    PUTBACK;\n", NULL);

    return params;
}

static char*
S_callback_refcount_mods(CFCMethod *method) {
    char *refcount_mods = CFCUtil_strdup("");
    CFCType *return_type = CFCMethod_get_return_type(method);
    CFCParamList *param_list = CFCMethod_get_param_list(method);
    CFCVariable **arg_vars = CFCParamList_get_variables(param_list);

    // `XSBind_perl_to_cfish()` returns an incremented object.  If this method
    // does not return an incremented object, we must cancel out that
    // refcount.  (No function can return a decremented object.)
    if (CFCType_is_object(return_type) && !CFCType_incremented(return_type)) {
        refcount_mods = CFCUtil_cat(refcount_mods,
                                    "\n    CFISH_DECREF(retval);", NULL);
    }

    // Adjust refcounts of arguments per method signature, so that Perl code
    // does not have to.
    for (int i = 0; arg_vars[i] != NULL; i++) {
        CFCVariable *var  = arg_vars[i];
        CFCType     *type = CFCVariable_get_type(var);
        const char  *name = CFCVariable_micro_sym(var);
        if (!CFCType_is_object(type)) {
            continue;
        }
        else if (CFCType_incremented(type)) {
            refcount_mods = CFCUtil_cat(refcount_mods, "\n    CFISH_INCREF(",
                                        name, ");", NULL);
        }
        else if (CFCType_decremented(type)) {
            refcount_mods = CFCUtil_cat(refcount_mods, "\n    CFISH_DECREF(",
                                        name, ");", NULL);
        }
    }

    return refcount_mods;
}

static char*
S_invalid_callback_def(CFCMethod *method) {
    const char *override_sym   = CFCMethod_full_override_sym(method);
    char *full_method_sym      = CFCMethod_full_method_sym(method, NULL);
    CFCType      *return_type  = CFCMethod_get_return_type(method);
    CFCParamList *param_list   = CFCMethod_get_param_list(method);
    const char   *ret_type_str = CFCType_to_c(return_type);
    const char   *params       = CFCParamList_to_c(param_list);
    char *maybe_ret
        = CFCType_is_void(return_type)
          ? CFCUtil_sprintf("")
          : CFCUtil_sprintf("CFISH_UNREACHABLE_RETURN(%s);\n", ret_type_str);
    char pattern[] =
        "%s\n"
        "%s(%s) {\n"
        "    CFISH_UNUSED_VAR(self);\n"
        "    cfish_Err_invalid_callback(\"%s\");\n"
        "%s"
        "}\n"
        ;
    char *callback_def = CFCUtil_sprintf(pattern, ret_type_str, override_sym,
                                         params, full_method_sym, maybe_ret);
    FREEMEM(full_method_sym);
    FREEMEM(maybe_ret);
    return callback_def;
}

static char*
S_void_callback_def(CFCMethod *method, const char *callback_start,
                    const char *refcount_mods) {
    const char *override_sym = CFCMethod_full_override_sym(method);
    const char *params = CFCParamList_to_c(CFCMethod_get_param_list(method));
    char *perl_name = CFCPerlMethod_perl_name(method);
    const char pattern[] =
        "void\n"
        "%s(%s) {\n"
        "%s"
        "    S_finish_callback_void(\"%s\");%s\n"
        "}\n";
    char *callback_def
        = CFCUtil_sprintf(pattern, override_sym, params, callback_start,
                          perl_name, refcount_mods);

    FREEMEM(perl_name);
    return callback_def;
}

static char*
S_primitive_callback_def(CFCMethod *method, const char *callback_start,
                         const char *refcount_mods) {
    const char *override_sym = CFCMethod_full_override_sym(method);
    const char *params = CFCParamList_to_c(CFCMethod_get_param_list(method));
    CFCType *return_type = CFCMethod_get_return_type(method);
    const char *ret_type_str = CFCType_to_c(return_type);
    char callback_func[50];

    if (CFCType_is_integer(return_type)) {
	if (strcmp(ret_type_str, "bool") == 0) {
             strcpy(callback_func, "!!S_finish_callback_i64");
	}
	else {
             strcpy(callback_func, "S_finish_callback_i64");
	}
    }
    else if (CFCType_is_floating(return_type)) {
        strcpy(callback_func, "S_finish_callback_f64");
    }
    else {
        CFCUtil_die("Unexpected type: %s", ret_type_str);
    }

    char *perl_name = CFCPerlMethod_perl_name(method);

    char pattern[] =
        "%s\n"
        "%s(%s) {\n"
        "%s"
        "    %s retval = (%s)%s(\"%s\");%s\n"
        "    return retval;\n"
        "}\n";
    char *callback_def
        = CFCUtil_sprintf(pattern, ret_type_str, override_sym, params,
                          callback_start, ret_type_str, ret_type_str,
                          callback_func, perl_name, refcount_mods);

    FREEMEM(perl_name);
    return callback_def;
}

static char*
S_obj_callback_def(CFCMethod *method, const char *callback_start,
                   const char *refcount_mods) {
    const char *override_sym = CFCMethod_full_override_sym(method);
    const char *params = CFCParamList_to_c(CFCMethod_get_param_list(method));
    CFCType *return_type = CFCMethod_get_return_type(method);
    const char *ret_type_str = CFCType_to_c(return_type);
    const char *nullable  = CFCType_nullable(return_type) ? "true" : "false";

    char *perl_name = CFCPerlMethod_perl_name(method);

    char pattern[] =
        "%s\n"
        "%s(%s) {\n"
        "%s"
        "    %s retval = (%s)S_finish_callback_obj(self, \"%s\", %s);%s\n"
        "    return retval;\n"
        "}\n";
    char *callback_def
        = CFCUtil_sprintf(pattern, ret_type_str, override_sym, params,
                          callback_start, ret_type_str, ret_type_str,
                          perl_name, nullable, refcount_mods);

    FREEMEM(perl_name);
    return callback_def;
}


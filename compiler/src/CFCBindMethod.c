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

#include <stdio.h>
#include <string.h>
#include "CFCBindMethod.h"
#include "CFCUtil.h"
#include "CFCMethod.h"
#include "CFCFunction.h"
#include "CFCParamList.h"
#include "CFCType.h"
#include "CFCVariable.h"
#include "CFCSymbol.h"
#include "CFCClass.h"

/* Create a macro definition that aliases to a function name directly, since
 * this method may not be overridden. */
static char*
S_final_method_def(CFCMethod *method, CFCClass *klass);

static char*
S_virtual_method_def(CFCMethod *method, CFCClass *klass);

char*
CFCBindMeth_method_def(CFCMethod *method, CFCClass *klass) {
    if (CFCMethod_final(method)) {
        return S_final_method_def(method, klass);
    }
    else {
        return S_virtual_method_def(method, klass);
    }
}

/* Create a macro definition that aliases to a function name directly, since
 * this method may not be overridden. */
static char*
S_final_method_def(CFCMethod *method, CFCClass *klass) {
    const char *self_type = CFCType_to_c(CFCMethod_self_type(method));
    const char *full_func_sym = CFCMethod_imp_func(method);
    const char *arg_names 
        = CFCParamList_name_list(CFCMethod_get_param_list(method));

    char *full_meth_sym   = CFCMethod_full_method_sym(method, klass);
    char *full_offset_sym = CFCMethod_full_offset_sym(method, klass);

    const char pattern[] =
        "extern size_t %s;\n"
        "#define %s(%s) \\\n"
        "    %s((%s)%s)\n";
    char *method_def
        = CFCUtil_sprintf(pattern, full_offset_sym, full_meth_sym, arg_names,
                          full_func_sym, self_type, arg_names);

    FREEMEM(full_offset_sym);
    FREEMEM(full_meth_sym);
    return method_def;
}

static char*
S_virtual_method_def(CFCMethod *method, CFCClass *klass) {
    CFCParamList *param_list = CFCMethod_get_param_list(method);
    const char *PREFIX         = CFCClass_get_PREFIX(klass);
    const char *invoker_struct = CFCClass_full_struct_sym(klass);

    char *full_meth_sym   = CFCMethod_full_method_sym(method, klass);
    char *full_offset_sym = CFCMethod_full_offset_sym(method, klass);
    char *full_typedef    = CFCMethod_full_typedef(method, klass);

    // Prepare parameter lists, minus invoker.  The invoker gets forced to
    // "self" later.
    if (CFCParamList_variadic(param_list)) {
        CFCUtil_die("Variadic methods not supported");
    }
    const char *arg_names_minus_invoker = CFCParamList_name_list(param_list);
    const char *params_minus_invoker    = CFCParamList_to_c(param_list);
    while (*arg_names_minus_invoker && *arg_names_minus_invoker != ',') {
        arg_names_minus_invoker++;
    }
    while (*params_minus_invoker && *params_minus_invoker != ',') {
        params_minus_invoker++;
    }

    // Prepare a return statement... or not.
    CFCType *return_type = CFCMethod_get_return_type(method);
    const char *ret_type_str = CFCType_to_c(return_type);
    const char *maybe_return = CFCType_is_void(return_type) ? "" : "return ";

    const char pattern[] =
        "extern %sVISIBLE size_t %s;\n"
        "static CFISH_INLINE %s\n"
        "%s(%s *self%s) {\n"
        "    const %s method = (%s)cfish_obj_method(self, %s);\n"
        "    %smethod(self%s);\n"
        "}\n";
    char *method_def
        = CFCUtil_sprintf(pattern, PREFIX, full_offset_sym, ret_type_str,
                          full_meth_sym, invoker_struct, params_minus_invoker,
                          full_typedef, full_typedef, full_offset_sym,
                          maybe_return, arg_names_minus_invoker);

    FREEMEM(full_offset_sym);
    FREEMEM(full_meth_sym);
    FREEMEM(full_typedef);
    return method_def;
}

char*
CFCBindMeth_typedef_dec(struct CFCMethod *method, CFCClass *klass) {
    const char *params_minus_invoker
        = CFCParamList_to_c(CFCMethod_get_param_list(method));
    while (*params_minus_invoker && *params_minus_invoker != ',') {
        params_minus_invoker++;
    }
    const char *self_struct = CFCClass_full_struct_sym(klass);
    const char *ret_type = CFCType_to_c(CFCMethod_get_return_type(method));
    char *full_typedef = CFCMethod_full_typedef(method, klass);
    char *buf = CFCUtil_sprintf("typedef %s\n(*%s)(%s *self%s);\n", ret_type,
                                full_typedef, self_struct,
                                params_minus_invoker);
    FREEMEM(full_typedef);
    return buf;
}

char*
CFCBindMeth_novel_spec_def(CFCMethod *method) {
    const char *meth_name = CFCMethod_get_name(method);
    const char *imp_func  = CFCMethod_imp_func(method);

    const char *full_override_sym = "NULL";
    if (!CFCMethod_final(method)) {
        full_override_sym = CFCMethod_full_override_sym(method);
    }

    char *full_offset_sym = CFCMethod_full_offset_sym(method, NULL);

    char pattern[] =
        "    {\n"
        "        &%s, /* offset */\n"
        "        \"%s\", /* name */\n"
        "        (cfish_method_t)%s, /* func */\n"
        "        (cfish_method_t)%s /* callback_func */\n"
        "    }";
    char *def
        = CFCUtil_sprintf(pattern, full_offset_sym, meth_name, imp_func,
                          full_override_sym);

    FREEMEM(full_offset_sym);
    return def;
}

char*
CFCBindMeth_overridden_spec_def(CFCMethod *method, CFCClass *klass) {
    const char *imp_func  = CFCMethod_imp_func(method);

    char *full_offset_sym = CFCMethod_full_offset_sym(method, NULL);

    CFCClass *parent = CFCClass_get_parent(klass);
    char *parent_offset_sym = CFCMethod_full_offset_sym(method, parent);

    char pattern[] =
        "    {\n"
        "        &%s, /* offset */\n"
        "        &%s, /* parent_offset */\n"
        "        (cfish_method_t)%s /* func */\n"
        "    }";
    char *def
        = CFCUtil_sprintf(pattern, full_offset_sym, parent_offset_sym,
                          imp_func);

    FREEMEM(full_offset_sym);
    FREEMEM(parent_offset_sym);
    return def;
}

char*
CFCBindMeth_inherited_spec_def(CFCMethod *method, CFCClass *klass) {
    char *full_offset_sym = CFCMethod_full_offset_sym(method, klass);

    CFCClass *parent = CFCClass_get_parent(klass);
    char *parent_offset_sym = CFCMethod_full_offset_sym(method, parent);

    char pattern[] =
        "    {\n"
        "        &%s, /* offset */\n"
        "        &%s /* parent_offset */\n"
        "    }";
    char *def = CFCUtil_sprintf(pattern, full_offset_sym, parent_offset_sym);

    FREEMEM(full_offset_sym);
    FREEMEM(parent_offset_sym);
    return def;
}

char*
CFCBindMeth_abstract_method_def(CFCMethod *method) {
    CFCType    *ret_type      = CFCMethod_get_return_type(method);
    const char *ret_type_str  = CFCType_to_c(ret_type);
    CFCType    *type          = CFCMethod_self_type(method);
    const char *full_func_sym = CFCMethod_imp_func(method);
    const char *class_var     = CFCType_get_class_var(type);
    const char *meth_name     = CFCMethod_get_name(method);
    CFCParamList *param_list  = CFCMethod_get_param_list(method);
    const char *params        = CFCParamList_to_c(param_list);
    CFCVariable **vars        = CFCParamList_get_variables(param_list);
    const char *invocant      = CFCVariable_get_name(vars[0]);

    // All variables other than the invocant are unused, and the return is
    // unreachable.
    char *unused = CFCUtil_strdup("");
    for (int i = 1; vars[i] != NULL; i++) {
        const char *var_name = CFCVariable_get_name(vars[i]);
        size_t size = strlen(unused) + strlen(var_name) + 80;
        unused = (char*)REALLOCATE(unused, size);
        strcat(unused, "\n    CFISH_UNUSED_VAR(");
        strcat(unused, var_name);
        strcat(unused, ");");
    }
    char *unreachable = CFCUtil_strdup("");
    if (!CFCType_is_void(ret_type)) {
        unreachable = CFCUtil_sprintf("    CFISH_UNREACHABLE_RETURN(%s);\n",
                                      ret_type_str);
    }

    char pattern[] =
        "%s\n"
        "%s(%s) {\n"
        "%s"
        "    cfish_Err_abstract_method_call((cfish_Obj*)%s, %s, \"%s\");\n"
        "%s"
        "}\n";
    char *abstract_def
        = CFCUtil_sprintf(pattern, ret_type_str, full_func_sym, params,
                          unused, invocant, class_var, meth_name,
                          unreachable);

    FREEMEM(unused);
    return abstract_def;
}

char*
CFCBindMeth_imp_declaration(CFCMethod *method) {
    CFCType      *return_type    = CFCMethod_get_return_type(method);
    CFCParamList *param_list     = CFCMethod_get_param_list(method);
    const char   *ret_type_str   = CFCType_to_c(return_type);
    const char   *full_imp_sym   = CFCMethod_imp_func(method);
    const char   *param_list_str = CFCParamList_to_c(param_list);
    char *buf = CFCUtil_sprintf("%s\n%s(%s);", ret_type_str,
                                full_imp_sym, param_list_str);
    return buf;
}


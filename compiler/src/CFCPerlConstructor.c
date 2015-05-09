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

#ifndef true
  #define true 1
  #define false 0
#endif

#define CFC_NEED_PERLSUB_STRUCT_DEF 1
#include "CFCPerlSub.h"
#include "CFCPerlConstructor.h"
#include "CFCClass.h"
#include "CFCFunction.h"
#include "CFCParamList.h"
#include "CFCType.h"
#include "CFCVariable.h"
#include "CFCUtil.h"
#include "CFCPerlTypeMap.h"

struct CFCPerlConstructor {
    CFCPerlSub   sub;
    CFCFunction *func;
    CFCClass    *klass;
};

static const CFCMeta CFCPERLCONSTRUCTOR_META = {
    "Clownfish::CFC::Binding::Perl::Constructor",
    sizeof(CFCPerlConstructor),
    (CFCBase_destroy_t)CFCPerlConstructor_destroy
};

CFCPerlConstructor*
CFCPerlConstructor_new(CFCClass *klass, const char *alias,
                       const char *initializer) {
    CFCPerlConstructor *self
        = (CFCPerlConstructor*)CFCBase_allocate(&CFCPERLCONSTRUCTOR_META);
    return CFCPerlConstructor_init(self, klass, alias, initializer);
}

CFCPerlConstructor*
CFCPerlConstructor_init(CFCPerlConstructor *self, CFCClass *klass,
                        const char *alias, const char *initializer) {
    CFCUTIL_NULL_CHECK(alias);
    CFCUTIL_NULL_CHECK(klass);
    self->klass = (CFCClass*)CFCBase_incref((CFCBase*)klass);
    const char *class_name = CFCClass_get_class_name(klass);
    if (!initializer) {
        if (CFCClass_final(klass)) {
            initializer = "new";
        }
        else {
            initializer = "init";
        }
    }

    // Find the implementing function.
    CFCFunction *func = CFCClass_function(klass, initializer);
    if (func) {
        self->func = (CFCFunction*)CFCBase_incref((CFCBase*)func);
    }
    else {
        CFCUtil_die("Missing or invalid '%s' function for '%s'",
                    initializer, class_name);
    }

    CFCParamList *param_list = CFCFunction_get_param_list(self->func);
    CFCPerlSub_init((CFCPerlSub*)self, param_list, class_name, alias,
                    true);
    return self;
}

void
CFCPerlConstructor_destroy(CFCPerlConstructor *self) {
    CFCBase_decref((CFCBase*)self->func);
    CFCBase_decref((CFCBase*)self->klass);
    CFCPerlSub_destroy((CFCPerlSub*)self);
}

char*
CFCPerlConstructor_xsub_def(CFCPerlConstructor *self) {
    const char *c_name = self->sub.c_name;
    CFCParamList *param_list = self->sub.param_list;
    char         *name_list  = CFCPerlSub_arg_name_list((CFCPerlSub*)self);
    CFCVariable **arg_vars   = CFCParamList_get_variables(param_list);
    const char   *func_sym   = CFCFunction_full_func_sym(self->func);
    const char   *struct_sym = CFCClass_full_struct_sym(self->klass);
    char *arg_decls = CFCPerlSub_arg_declarations((CFCPerlSub*)self, 0);

    char declarations_pattern[] =
        "    dXSARGS;\n"
        "    bool args_ok;\n"
        "    %s *retval;\n"
        "%s"
        ;
    char *declarations
        = CFCUtil_sprintf(declarations_pattern, struct_sym, arg_decls);

    // Compensate for swallowed refcounts.
    char *refcount_mods = CFCUtil_strdup("");
    for (size_t i = 0; arg_vars[i] != NULL; i++) {
        CFCVariable *var = arg_vars[i];
        CFCType *type = CFCVariable_get_type(var);
        if (CFCType_is_object(type) && CFCType_decremented(type)) {
            const char *name = CFCVariable_get_name(var);
            refcount_mods
                = CFCUtil_cat(refcount_mods, "\n    CFISH_INCREF(arg_", name,
                              ");", NULL);
        }
    }

    char *build_params;
    if (CFCClass_final(self->klass)) {
        build_params = CFCPerlSub_build_allot_params((CFCPerlSub*)self, 0);
    }
    else {
        build_params = CFCPerlSub_build_allot_params((CFCPerlSub*)self, 1);
        // Create "self" last, so that earlier exceptions while fetching
        // params don't trigger a bad invocation of DESTROY.
        char *temp = CFCUtil_sprintf(
            "%s\n    arg_self = (%s*)XSBind_new_blank_obj(aTHX_ ST(0));",
            build_params, struct_sym);
        FREEMEM(build_params);
        build_params = temp;
    }

    const char pattern[] =
        "XS(%s);\n"
        "XS(%s) {\n"
        "%s"       // declarations
        "\n"
        "    CFISH_UNUSED_VAR(cv);\n"
        "    if (items < 1) { CFISH_THROW(CFISH_ERR, \"Usage: %%s(class_name, ...)\",  GvNAME(CvGV(cv))); }\n"
        "    SP -= items;\n"
        "\n"
        "    %s\n" // build_params
        "%s"       // refcount_mods
        "\n"
        "    retval = %s(%s);\n"
        "    if (retval) {\n"
        "        ST(0) = (SV*)CFISH_Obj_To_Host((cfish_Obj*)retval);\n"
        "        CFISH_DECREF_NN(retval);\n"
        "    }\n"
        "    else {\n"
        "        ST(0) = newSV(0);\n"
        "    }\n"
        "    sv_2mortal(ST(0));\n"
        "    XSRETURN(1);\n"
        "}\n\n";
    char *xsub_def
        = CFCUtil_sprintf(pattern, c_name, c_name, declarations, build_params,
                          refcount_mods, func_sym, name_list);

    FREEMEM(refcount_mods);
    FREEMEM(declarations);
    FREEMEM(arg_decls);
    FREEMEM(build_params);
    FREEMEM(name_list);

    return xsub_def;
}


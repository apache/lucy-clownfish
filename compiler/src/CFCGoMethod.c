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

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCGoFunc.h"
#include "CFCGoMethod.h"
#include "CFCUtil.h"
#include "CFCClass.h"
#include "CFCFunction.h"
#include "CFCMethod.h"
#include "CFCSymbol.h"
#include "CFCType.h"
#include "CFCParcel.h"
#include "CFCParamList.h"
#include "CFCGoTypeMap.h"
#include "CFCVariable.h"

#ifndef true
    #define true 1
    #define false 0
#endif

struct CFCGoMethod {
    CFCBase     base;
    CFCMethod  *method;
    char       *sig;
};

static void
S_CFCGoMethod_destroy(CFCGoMethod *self);

static const CFCMeta CFCGOMETHOD_META = {
    "Clownfish::CFC::Binding::Go::Method",
    sizeof(CFCGoMethod),
    (CFCBase_destroy_t)S_CFCGoMethod_destroy
};

CFCGoMethod*
CFCGoMethod_new(CFCMethod *method) {
    CFCGoMethod *self
        = (CFCGoMethod*)CFCBase_allocate(&CFCGOMETHOD_META);
    self->method = (CFCMethod*)CFCBase_incref((CFCBase*)method);
    self->sig    = NULL;
    return self;
}

static void
S_CFCGoMethod_destroy(CFCGoMethod *self) {
    CFCBase_decref((CFCBase*)self->method);
    FREEMEM(self->sig);
    CFCBase_destroy((CFCBase*)self);
}

CFCMethod*
CFCGoMethod_get_client(CFCGoMethod *self) {
    return self->method;
}

void
CFCGoMethod_customize(CFCGoMethod *self, const char *sig) {
    FREEMEM(self->sig);
    self->sig = CFCUtil_strdup(sig);
    if (self->method) {
        CFCMethod_exclude_from_host(self->method);
    }
}

static void
S_lazy_init_sig(CFCGoMethod *self, CFCClass *invoker) {
    if (self->sig || !self->method) {
        return;
    }

    CFCMethod *method = self->method;
    CFCParcel *parcel = CFCClass_get_parcel(invoker);
    CFCType *return_type = CFCMethod_get_return_type(method);
    char *name = CFCGoFunc_go_meth_name(CFCMethod_get_name(method));
    char *go_ret_type = CFCType_is_void(return_type)
                        ? CFCUtil_strdup("")
                        : CFCGoTypeMap_go_type_name(return_type, parcel);

    // Assemble list of argument types.
    char *args = CFCUtil_strdup("");
    CFCParamList *param_list = CFCMethod_get_param_list(method);
    CFCVariable **vars = CFCParamList_get_variables(param_list);
    for (int i = 1; vars[i] != NULL; i++) {
        CFCType *type = CFCVariable_get_type(vars[i]);
        if (i > 1) {
            args = CFCUtil_cat(args, ", ", NULL);
        }
        char *go_type = CFCGoTypeMap_go_type_name(type, parcel);
        args = CFCUtil_cat(args, go_type, NULL);
        FREEMEM(go_type);
    }

    self->sig = CFCUtil_sprintf("%s(%s) %s", name, args, go_ret_type);

    FREEMEM(args);
    FREEMEM(go_ret_type);
    FREEMEM(name);
}

const char*
CFCGoMethod_get_sig(CFCGoMethod *self, CFCClass *invoker) {
    if (self->sig) {
        return self->sig;
    }
    else if (!self->method) {
        return "";
    }
    else {
        S_lazy_init_sig(self, invoker);
        return self->sig;
    }
}

#define GO_NAME_BUF_SIZE 128

static char*
S_prep_cfargs(CFCClass *invoker, CFCParamList *param_list) {
    CFCVariable **vars = CFCParamList_get_variables(param_list);
    char go_name[GO_NAME_BUF_SIZE];
    char *cfargs = CFCUtil_strdup("");

    for (int i = 0; vars[i] != NULL; i++) {
        CFCVariable *var = vars[i];
        CFCType *type = CFCVariable_get_type(var);
        if (i == 0) {
            CFCGoTypeMap_go_meth_receiever(CFCClass_get_struct_sym(invoker),
                                           param_list, go_name,
                                           GO_NAME_BUF_SIZE);
        }
        else {
            cfargs = CFCUtil_cat(cfargs, ", ", NULL);
            CFCGoTypeMap_go_arg_name(param_list, i, go_name, GO_NAME_BUF_SIZE);
        }

        if (CFCType_is_primitive(type)) {
            cfargs = CFCUtil_cat(cfargs, "C.", CFCType_get_specifier(type),
                                 "(", go_name, ")", NULL);
        }
        else if (CFCType_is_object(type)) {

            char *obj_pattern;
            if (CFCType_decremented(type)) {
                obj_pattern = "(*C.%s)(unsafe.Pointer(C.cfish_inc_refcount(unsafe.Pointer(%s.TOPTR()))))";
            }
            else {
                obj_pattern = "(*C.%s)(unsafe.Pointer(%s.TOPTR()))";
            }
            char *temp = CFCUtil_sprintf(obj_pattern,
                                         CFCType_get_specifier(type), go_name);
            cfargs = CFCUtil_cat(cfargs, temp, NULL);
            FREEMEM(temp);
        }
    }
    return cfargs;
}

char*
CFCGoMethod_func_def(CFCGoMethod *self, CFCClass *invoker) {
    if (!self->method || CFCMethod_excluded_from_host(self->method)) {
        return CFCUtil_strdup("");
    }

    CFCMethod    *novel_method = CFCMethod_find_novel_method(self->method);
    CFCParcel    *parcel     = CFCClass_get_parcel(invoker);
    CFCParamList *param_list = CFCMethod_get_param_list(novel_method);
    CFCType      *ret_type   = CFCMethod_get_return_type(novel_method);
    char *name = CFCGoFunc_go_meth_name(CFCMethod_get_name(novel_method));
    char *first_line = CFCGoFunc_func_start(parcel, name, invoker,
                                            param_list, ret_type, true);
    char *cfunc;
    if (CFCMethod_novel(self->method) && CFCMethod_final(self->method)) {
        cfunc = CFCUtil_strdup(CFCMethod_imp_func(self->method, invoker));
    }
    else {
        cfunc = CFCMethod_full_method_sym(novel_method, invoker);
    }

    char *cfargs = S_prep_cfargs(invoker, param_list);

    char *ret_type_str;
    char *maybe_retval;
    char *maybe_return;
    if (CFCType_is_void(ret_type)) {
        ret_type_str = CFCUtil_strdup("");
        maybe_retval = CFCUtil_strdup("");
        maybe_return = CFCUtil_strdup("");
    }
    else {
        ret_type_str = CFCGoTypeMap_go_type_name(ret_type, parcel);
        if (ret_type_str == NULL) {
            CFCUtil_die("Can't convert invalid type in method %s", name);
        }
        maybe_retval = CFCUtil_strdup("retvalCF := ");

        if (CFCType_is_primitive(ret_type)) {
            maybe_return = CFCUtil_sprintf("\treturn %s(retvalCF)\n", ret_type_str);
        }
        else if (CFCType_is_object(ret_type)) {
            char *go_type_name = CFCGoTypeMap_go_type_name(ret_type, parcel);
            char *struct_name  = go_type_name;
            char *go_package   = CFCUtil_strdup(go_type_name);
            for (int i = strlen(go_package) - 1; i >= 0; i--) {
                if (go_package[i] == '.') {
                    struct_name += i + 1;
                    break;
                }
                go_package[i] = '\0';
            }
            char *pattern;
            if (CFCType_incremented(ret_type)) {
                pattern = "\treturn %sWRAP%s(unsafe.Pointer(retvalCF))\n";
            }
            else {
                pattern = "\treturn %sWRAP%s(unsafe.Pointer(C.cfish_inc_refcount(unsafe.Pointer(retvalCF))))\n";
            }
            maybe_return = CFCUtil_sprintf(pattern, go_package, struct_name);
            FREEMEM(go_type_name);
            FREEMEM(go_package);
        }
        else {
            CFCUtil_die("Unexpected type: %s", CFCType_to_c(ret_type));
        }
    }

    char pattern[] =
        "%s"
        "\t%sC.%s(%s)\n"
        "%s"
        "}\n"
        ;
    char *content = CFCUtil_sprintf(pattern, first_line, maybe_retval,
                                    cfunc, cfargs, maybe_return);

    FREEMEM(maybe_retval);
    FREEMEM(maybe_return);
    FREEMEM(ret_type_str);
    FREEMEM(cfunc);
    FREEMEM(cfargs);
    FREEMEM(first_line);
    FREEMEM(name);
    return content;
}


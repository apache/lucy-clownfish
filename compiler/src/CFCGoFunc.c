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
#include "CFCGoFunc.h"
#include "CFCGoTypeMap.h"
#include "CFCBase.h"
#include "CFCClass.h"
#include "CFCFunction.h"
#include "CFCUtil.h"
#include "CFCParcel.h"
#include "CFCParamList.h"
#include "CFCVariable.h"
#include "CFCType.h"

#ifndef true
    #define true 1
    #define false 0
#endif

#define GO_NAME_BUF_SIZE 128

enum {
    IS_METHOD = 1,
    IS_FUNC   = 2,
    IS_CTOR   = 3
};

char*
CFCGoFunc_go_meth_name(const char *orig) {
    char *go_name = CFCUtil_strdup(orig);
    for (int i = 0, j = 0, max = strlen(go_name) + 1; i < max; i++) {
        if (go_name[i] != '_') {
            go_name[j++] = go_name[i];
        }
    }
    return go_name;
}

static char*
S_prep_start(CFCParcel *parcel, const char *name, CFCClass *invoker,
             CFCParamList *param_list, CFCType *return_type, int targ) {
    CFCVariable **param_vars = CFCParamList_get_variables(param_list);
    char *invocant;
    char go_name[GO_NAME_BUF_SIZE];

    if (targ == IS_METHOD) {
        const char *struct_sym = CFCClass_get_struct_sym(invoker);
        CFCGoTypeMap_go_meth_receiever(struct_sym, param_list, go_name,
                                       GO_NAME_BUF_SIZE);
        invocant = CFCUtil_sprintf("(%s *%sIMP) ", go_name, struct_sym);
    }
    else {
        invocant = CFCUtil_strdup("");
    }

    char *params = CFCUtil_strdup("");
    int start = targ == IS_METHOD ? 1 : 0;
    for (int i = start; param_vars[i] != NULL; i++) {
        CFCVariable *var = param_vars[i];
        CFCType *type = CFCVariable_get_type(var);
        char *go_type_name = CFCGoTypeMap_go_type_name(type, parcel);
        CFCGoTypeMap_go_arg_name(param_list, i, go_name, GO_NAME_BUF_SIZE);
        if (i > start) {
            params = CFCUtil_cat(params, ", ", NULL);
        }
        params = CFCUtil_cat(params, go_name, " ", go_type_name, NULL);
        FREEMEM(go_type_name);
    }

    char *ret_type_str;
    if (CFCType_is_void(return_type)) {
        ret_type_str = CFCUtil_strdup("");
    }
    else {
        ret_type_str = CFCGoTypeMap_go_type_name(return_type, parcel);
        if (ret_type_str == NULL) {
            CFCUtil_die("Can't convert invalid type in method %s", name);
        }
    }

    char pattern[] =
        "func %s%s(%s) %s {\n"
    ;
    char *content
        = CFCUtil_sprintf(pattern, invocant, name, params, ret_type_str);

    FREEMEM(invocant);
    FREEMEM(params);
    FREEMEM(ret_type_str);
    return content;
}

char*
CFCGoFunc_meth_start(CFCParcel *parcel, const char *name, CFCClass *invoker,
                     CFCParamList *param_list, CFCType *return_type) {
    return S_prep_start(parcel, name, invoker, param_list, return_type,
                        IS_METHOD);
}

char*
CFCGoFunc_ctor_start(CFCParcel *parcel, const char *name,
                     CFCParamList *param_list, CFCType *return_type) {
    return S_prep_start(parcel, name, NULL, param_list, return_type,
                        IS_CTOR);
}

static char*
S_prep_cfargs(CFCParcel *parcel, CFCClass *invoker,
                      CFCParamList *param_list, int targ) {
    CFCVariable **vars = CFCParamList_get_variables(param_list);
    char go_name[GO_NAME_BUF_SIZE];
    char *cfargs = CFCUtil_strdup("");

    for (int i = 0; vars[i] != NULL; i++) {
        CFCVariable *var = vars[i];
        CFCType *type = CFCVariable_get_type(var);
        if (targ == IS_METHOD && i == 0) {
            CFCGoTypeMap_go_meth_receiever(CFCClass_get_struct_sym(invoker),
                                           param_list, go_name,
                                           GO_NAME_BUF_SIZE);
        }
        else {
            CFCGoTypeMap_go_arg_name(param_list, i, go_name, GO_NAME_BUF_SIZE);
        }

        if (i > 0) {
            cfargs = CFCUtil_cat(cfargs, ", ", NULL);
        }

        if (CFCType_is_primitive(type)) {
            cfargs = CFCUtil_cat(cfargs, "C.", CFCType_get_specifier(type),
                                 "(", go_name, ")", NULL);
        }
        else if (CFCType_is_string_type(type)
                 // Don't convert a clownfish.String invocant.
                 && (targ != IS_METHOD || i != 0)
                ) {
            const char *format;
            if (CFCParcel_is_cfish(parcel)) {
                format = "%s((*C.cfish_String)(unsafe.Pointer(NewString(%s).TOPTR())))";
            }
            else {
                format = "%s((*C.cfish_String)(unsafe.Pointer(clownfish.NewString(%s).TOPTR())))";
            }
            char *temp = CFCUtil_sprintf(format, cfargs, go_name);
            FREEMEM(cfargs);
            cfargs = temp;
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
CFCGoFunc_meth_cfargs(CFCParcel *parcel, CFCClass *invoker,
                      CFCParamList *param_list) {
    return S_prep_cfargs(parcel, invoker, param_list, IS_METHOD);
}

char*
CFCGoFunc_ctor_cfargs(CFCParcel *parcel, CFCParamList *param_list) {
    return S_prep_cfargs(parcel, NULL, param_list, IS_CTOR);
}

char*
CFCGoFunc_return_statement(CFCParcel *parcel, CFCType *return_type,
                           const char *cf_retval) {
    char *statement = NULL;
    if (CFCType_is_void(return_type)) {
        return CFCUtil_strdup("");
    }
    else {
        char *ret_type_str = CFCGoTypeMap_go_type_name(return_type, parcel);
        if (ret_type_str == NULL) {
            CFCUtil_die("Can't convert type to Go: %s",
                        CFCType_to_c(return_type));
        }

        if (CFCType_is_primitive(return_type)) {
            statement = CFCUtil_sprintf("\treturn %s(retvalCF)\n", ret_type_str);
        }
        else if (CFCType_is_string_type(return_type)) {
            const char *clownfish_dot = CFCParcel_is_cfish(parcel)
                                        ? "" : "clownfish.";
            if (CFCType_incremented(return_type)) {
                char pattern[] =
                    "\tdefer C.cfish_dec_refcount(unsafe.Pointer(retvalCF))\n"
                    "\treturn %sCFStringToGo(unsafe.Pointer(retvalCF))\n"
                    ;
                statement = CFCUtil_sprintf(pattern, clownfish_dot);
            }
            else {
                char pattern[] =
                    "\treturn %sCFStringToGo(unsafe.Pointer(retvalCF))\n";
                statement = CFCUtil_sprintf(pattern, clownfish_dot);
            }
        }
        else if (CFCType_is_object(return_type)) {
            char *go_type_name = CFCGoTypeMap_go_type_name(return_type, parcel);
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
            if (CFCType_incremented(return_type)) {
                pattern = "\treturn %sWRAP%s(unsafe.Pointer(retvalCF))\n";
            }
            else {
                pattern = "\treturn %sWRAP%s(unsafe.Pointer(C.cfish_inc_refcount(unsafe.Pointer(retvalCF))))\n";
            }
            statement = CFCUtil_sprintf(pattern, go_package, struct_name);
            FREEMEM(go_type_name);
            FREEMEM(go_package);
        }
        else {
            CFCUtil_die("Unexpected type: %s", CFCType_to_c(return_type));
        }
    }

    return statement;
}

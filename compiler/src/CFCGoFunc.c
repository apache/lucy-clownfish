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

char*
CFCGoFunc_func_start(CFCParcel *parcel, const char *name,
                     CFCParamList *param_list, CFCType *return_type,
                     int is_method) {
    CFCVariable **param_vars = CFCParamList_get_variables(param_list);
    char *invocant;
    if (is_method) {
        CFCVariable *var = param_vars[0];
        CFCType *type = CFCVariable_get_type(var);
        char *go_type_name = CFCGoTypeMap_go_type_name(type, parcel);
        invocant = CFCUtil_sprintf("(%s *%sIMP) ", CFCVariable_micro_sym(var),
                                   go_type_name);
        FREEMEM(go_type_name);
    }
    else {
        invocant = CFCUtil_strdup("");
    }

    char *params = CFCUtil_strdup("");
    int start = is_method ? 1 : 0;
    for (int i = start; param_vars[i] != NULL; i++) {
        CFCVariable *var = param_vars[i];
        CFCType *type = CFCVariable_get_type(var);
        char *go_type_name = CFCGoTypeMap_go_type_name(type, parcel);
        if (i > start) {
            params = CFCUtil_cat(params, ", ", NULL);
        }
        params = CFCUtil_cat(params, CFCVariable_micro_sym(var), " ",
                             go_type_name, NULL);
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


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

#define C_CFISH_METHOD
#define CFISH_USE_SHORT_NAMES

#include "Clownfish/Method.h"
#include "Clownfish/String.h"
#include "Clownfish/Err.h"
#include "Clownfish/Class.h"

Method*
Method_new(String *name, cfish_method_t callback_func, size_t offset) {
    Method *self = (Method*)Class_Make_Obj(METHOD);
    return Method_init(self, name, callback_func, offset);
}

Method*
Method_init(Method *self, String *name, cfish_method_t callback_func,
            size_t offset) {
    /* The `name` member which Method exposes via the `Get_Name` accessor uses
     * a "wrapped" string because that is effectively threadsafe: an INCREF
     * results in a copy and the only reference is owned by an immortal
     * object. */
    self->name_internal = Str_Clone(name);
    self->name
        = Str_new_wrap_trusted_utf8(Str_Get_Ptr8(self->name_internal),
                                    Str_Get_Size(self->name_internal));

    self->host_alias    = NULL;
    self->callback_func = callback_func;
    self->offset        = offset;
    self->is_excluded   = false;

    return self;
}

void
Method_Destroy_IMP(Method *self) {
    THROW(ERR, "Insane attempt to destroy Method '%o'", self->name);
}

String*
Method_Get_Name_IMP(Method *self) {
    return self->name;
}

void
Method_Set_Host_Alias_IMP(Method *self, String *name) {
    if (self->host_alias) {
        THROW(ERR, "Can't Set_Host_Alias more than once");
    }
    self->host_alias_internal = Str_Clone(name);
    self->host_alias
        = Str_new_wrap_trusted_utf8(Str_Get_Ptr8(self->host_alias_internal),
                                    Str_Get_Size(self->host_alias_internal));
}

String*
Method_Get_Host_Alias_IMP(Method *self) {
    return self->host_alias;
}

bool
Method_Is_Excluded_From_Host_IMP(Method *self) {
    return self->is_excluded;
}



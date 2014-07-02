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

#define CHY_USE_SHORT_NAMES
#define CFISH_USE_SHORT_NAMES
#define C_CFISH_OBJ
#define C_CFISH_CLASS

#include "charmony.h"

#include "Clownfish/Class.h"
#include "Clownfish/String.h"
#include "Clownfish/Err.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/VArray.h"

Obj*
Class_Make_Obj_IMP(Class *self) {
    Obj *obj = (Obj*)Memory_wrapped_calloc(self->obj_alloc_size, 1);
    obj->klass = self;
    obj->refcount = 1;
    return obj;
}

Obj*
Class_Init_Obj_IMP(Class *self, void *allocation) {
    Obj *obj = (Obj*)allocation;
    obj->klass = self;
    obj->refcount = 1;
    return obj;
}

Obj*
Class_Foster_Obj_IMP(Class *self, void *host_obj) {
    UNUSED_VAR(self);
    UNUSED_VAR(host_obj);
    THROW(ERR, "TODO");
    UNREACHABLE_RETURN(Obj*);
}

void
Class_register_with_host(Class *singleton, Class *parent) {
    UNUSED_VAR(singleton);
    UNUSED_VAR(parent);
}

VArray*
Class_fresh_host_methods(String *class_name) {
    UNUSED_VAR(class_name);
    return VA_new(0);
}

String*
Class_find_parent_class(String *class_name) {
    UNUSED_VAR(class_name);
    THROW(ERR, "TODO");
    UNREACHABLE_RETURN(String*);
}

void*
Class_To_Host_IMP(Class *self) {
    UNUSED_VAR(self);
    THROW(ERR, "TODO");
    UNREACHABLE_RETURN(void*);
}


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

#define C_CFISH_OBJ
#define C_CFISH_CLASS

#include "CFBind.h"

cfish_Obj*
CFISH_Class_Make_Obj_IMP(cfish_Class *self) {
    THROW(CFISH_ERR, "TODO");
    UNREACHABLE_RETURN(cfish_Obj*);
}

cfish_Obj*
CFISH_Class_Init_Obj_IMP(cfish_Class *self, void *allocation) {
    THROW(CFISH_ERR, "TODO");
    UNREACHABLE_RETURN(cfish_Obj*);
}

cfish_Obj*
CFISH_Class_Foster_Obj_IMP(cfish_Class *self, void *host_obj) {
    THROW(CFISH_ERR, "TODO");
    UNREACHABLE_RETURN(cfish_Obj*);
}

void
cfish_Class_register_with_host(cfish_Class *singleton, cfish_Class *parent) {
    THROW(CFISH_ERR, "TODO");
}

cfish_Vector*
cfish_Class_fresh_host_methods(const cfish_String *class_name) {
    THROW(CFISH_ERR, "TODO");
    UNREACHABLE_RETURN(cfish_Vector*);
}

cfish_String*
cfish_Class_find_parent_class(const cfish_String *class_name) {
    THROW(CFISH_ERR, "TODO");
    UNREACHABLE_RETURN(cfish_String*);
}


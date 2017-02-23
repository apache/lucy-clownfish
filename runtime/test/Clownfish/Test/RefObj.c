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

#define C_TESTCFISH_REFOBJ
#define CFISH_USE_SHORT_NAMES
#define TESTCFISH_USE_SHORT_NAMES

#include "Clownfish/Test/RefObj.h"
#include "Clownfish/Class.h"
#include "Clownfish/Err.h"

RefObj*
RefObj_new() {
    return (RefObj*)Class_Make_Obj(REFOBJ);
}

void
RefObj_Set_Ref_IMP(RefObj *self, Obj *other) {
    RefObjIVARS *const ivars = RefObj_IVARS(self);
    Obj *temp = ivars->ref;
    ivars->ref = INCREF(other);
    DECREF(temp);
}

void
RefObj_Destroy_IMP(RefObj *self) {
    RefObjIVARS *const ivars = RefObj_IVARS(self);
    if (cfish_get_refcount(self) > 1) {
        THROW(ERR, "Destroy called on referenced object");
    }
    DECREF(ivars->ref);
    SUPER_DESTROY(self, REFOBJ);
}


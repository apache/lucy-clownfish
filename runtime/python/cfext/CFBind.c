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
#define C_CFISH_METHOD
#define C_CFISH_ERR

#include "charmony.h"
#include "CFBind.h"

#include "Clownfish/Obj.h"
#include "Clownfish/Class.h"
#include "Clownfish/Blob.h"
#include "Clownfish/Boolean.h"
#include "Clownfish/ByteBuf.h"
#include "Clownfish/Err.h"
#include "Clownfish/Hash.h"
#include "Clownfish/Method.h"
#include "Clownfish/Num.h"
#include "Clownfish/String.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Vector.h"

/**** refcounting **********************************************************/

uint32_t
cfish_get_refcount(void *vself) {
    return Py_REFCNT(vself);
}

cfish_Obj*
cfish_inc_refcount(void *vself) {
    Py_INCREF(vself);
    return (cfish_Obj*)vself;
}

uint32_t
cfish_dec_refcount(void *vself) {
    uint32_t modified_refcount = Py_REFCNT(vself);
    Py_DECREF(vself);
    return modified_refcount;
}

/**** Obj ******************************************************************/

void*
CFISH_Obj_To_Host_IMP(cfish_Obj *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

/**** Class ****************************************************************/

cfish_Obj*
CFISH_Class_Make_Obj_IMP(cfish_Class *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Obj*);
}

cfish_Obj*
CFISH_Class_Init_Obj_IMP(cfish_Class *self, void *allocation) {
    CFISH_UNUSED_VAR(self);
    CFISH_UNUSED_VAR(allocation);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Obj*);
}

void
cfish_Class_register_with_host(cfish_Class *singleton, cfish_Class *parent) {
    CFISH_UNUSED_VAR(singleton);
    CFISH_UNUSED_VAR(parent);
    CFISH_THROW(CFISH_ERR, "TODO");
}

cfish_Vector*
cfish_Class_fresh_host_methods(cfish_String *class_name) {
    CFISH_UNUSED_VAR(class_name);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Vector*);
}

cfish_String*
cfish_Class_find_parent_class(cfish_String *class_name) {
    CFISH_UNUSED_VAR(class_name);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_String*);
}

/**** Method ***************************************************************/

cfish_String*
CFISH_Method_Host_Name_IMP(cfish_Method *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_String*);
}

/**** Err ******************************************************************/

void
cfish_Err_init_class() {
    CFISH_THROW(CFISH_ERR, "TODO");
}

cfish_Err*
cfish_Err_get_error() {
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Err*);
}

void
cfish_Err_set_error(cfish_Err *error) {
    CFISH_UNUSED_VAR(error);
    CFISH_THROW(CFISH_ERR, "TODO");
}

void
cfish_Err_do_throw(cfish_Err *error) {
    CFISH_UNUSED_VAR(error);
    CFISH_THROW(CFISH_ERR, "TODO");
}

void
cfish_Err_throw_mess(cfish_Class *klass, cfish_String *message) {
    CFISH_UNUSED_VAR(klass);
    CFISH_UNUSED_VAR(message);
    CFISH_THROW(CFISH_ERR, "TODO");
}

void
cfish_Err_warn_mess(cfish_String *message) {
    CFISH_UNUSED_VAR(message);
    CFISH_THROW(CFISH_ERR, "TODO");
}

cfish_Err*
cfish_Err_trap(CFISH_Err_Attempt_t routine, void *routine_context) {
    CFISH_UNUSED_VAR(routine);
    CFISH_UNUSED_VAR(routine_context);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Err*);
}

/**** TestUtils ************************************************************/

void*
cfish_TestUtils_clone_host_runtime() {
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void
cfish_TestUtils_set_host_runtime(void *runtime) {
    CFISH_UNUSED_VAR(runtime);
    CFISH_THROW(CFISH_ERR, "TODO");
}

void
cfish_TestUtils_destroy_host_runtime(void *runtime) {
    CFISH_UNUSED_VAR(runtime);
    CFISH_THROW(CFISH_ERR, "TODO");
}

/**** To_Host methods ******************************************************/

void*
CFISH_Str_To_Host_IMP(cfish_String *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Blob_To_Host_IMP(cfish_Blob *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_BB_To_Host_IMP(cfish_ByteBuf *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Vec_To_Host_IMP(cfish_Vector *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Hash_To_Host_IMP(cfish_Hash *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Float_To_Host_IMP(cfish_Float *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Int_To_Host_IMP(cfish_Integer *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Bool_To_Host_IMP(cfish_Boolean *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}


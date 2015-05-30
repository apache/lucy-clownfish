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

#define C_CFISH_CLASS
#define C_CFISH_METHOD
#define C_CFISH_OBJ
#define CFISH_USE_SHORT_NAMES

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charmony.h"

#include "tls.h"
#include "Clownfish/Obj.h"
#include "Clownfish/Class.h"
#include "Clownfish/Blob.h"
#include "Clownfish/ByteBuf.h"
#include "Clownfish/Err.h"
#include "Clownfish/Hash.h"
#include "Clownfish/Method.h"
#include "Clownfish/Num.h"
#include "Clownfish/String.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Vector.h"

/**** Obj ******************************************************************/

static CFISH_INLINE bool
SI_immortal(cfish_Class *klass) {
    if (klass == CFISH_CLASS
        || klass == CFISH_METHOD
        || klass == CFISH_BOOLNUM
       ){
        return true;
    }
    return false;
}

static CFISH_INLINE bool
SI_is_string_type(cfish_Class *klass) {
    if (klass == CFISH_STRING) {
        return true;
    }
    return false;
}

uint32_t
cfish_get_refcount(void *vself) {
    cfish_Obj *self = (cfish_Obj*)vself;
    return self->refcount;
}

Obj*
cfish_inc_refcount(void *vself) {
    Obj *self = (Obj*)vself;

    // Handle special cases.
    cfish_Class *const klass = self->klass;
    if (klass->flags & CFISH_fREFCOUNTSPECIAL) {
        if (SI_is_string_type(klass)) {
            // Only copy-on-incref Strings get special-cased.  Ordinary
            // strings fall through to the general case.
            if (CFISH_Str_Is_Copy_On_IncRef((cfish_String*)self)) {
                const char *utf8 = CFISH_Str_Get_Ptr8((cfish_String*)self);
                size_t size = CFISH_Str_Get_Size((cfish_String*)self);
                return (cfish_Obj*)cfish_Str_new_from_trusted_utf8(utf8, size);
            }
        }
        else if (SI_immortal(klass)) {
            return self;
        }
    }

    self->refcount++;
    return self;
}

uint32_t
cfish_dec_refcount(void *vself) {
    cfish_Obj *self = (Obj*)vself;
    cfish_Class *klass = self->klass;
    if (klass->flags & CFISH_fREFCOUNTSPECIAL) {
        if (SI_immortal(klass)) {
            return self->refcount;
        }
    }

    uint32_t modified_refcount = INT32_MAX;
    switch (self->refcount) {
        case 0:
            THROW(ERR, "Illegal refcount of 0");
            break; // useless
        case 1:
            modified_refcount = 0;
            Obj_Destroy(self);
            break;
        default:
            modified_refcount = --self->refcount;
            break;
    }
    return modified_refcount;
}

void*
Obj_To_Host_IMP(Obj *self) {
    UNUSED_VAR(self);
    THROW(ERR, "Obj_To_Host not supported in C bindings");
    UNREACHABLE_RETURN(void*);
}

/**** Class ****************************************************************/

Obj*
Class_Make_Obj_IMP(Class *self) {
    Obj *obj = (Obj*)Memory_wrapped_calloc(self->obj_alloc_size, 1);
    obj->klass = self;
    obj->refcount = 1;
    return obj;
}

Obj*
Class_Init_Obj_IMP(Class *self, void *allocation) {
    memset(allocation, 0, self->obj_alloc_size);
    Obj *obj = (Obj*)allocation;
    obj->klass = self;
    obj->refcount = 1;
    return obj;
}

Obj*
Class_Foster_Obj_IMP(Class *self, void *host_obj) {
    UNUSED_VAR(self);
    UNUSED_VAR(host_obj);
    THROW(ERR, "Class_Foster_Obj not supported in C bindings");
    UNREACHABLE_RETURN(Obj*);
}

void
Class_register_with_host(Class *singleton, Class *parent) {
    UNUSED_VAR(singleton);
    UNUSED_VAR(parent);
}

Vector*
Class_fresh_host_methods(String *class_name) {
    UNUSED_VAR(class_name);
    return Vec_new(0);
}

String*
Class_find_parent_class(String *class_name) {
    UNUSED_VAR(class_name);
    THROW(ERR, "Class_find_parent_class not supported in C bindings");
    UNREACHABLE_RETURN(String*);
}

/**** Method ***************************************************************/

String*
Method_Host_Name_IMP(Method *self) {
    return (String*)INCREF(self->name);
}

/**** Err ******************************************************************/

void
Err_init_class() {
    Tls_init();
}

Err*
Err_get_error() {
    return Tls_get_err_context()->current_error;
}

void
Err_set_error(Err *error) {
    ErrContext *context = Tls_get_err_context();

    if (context->current_error) {
        DECREF(context->current_error);
    }
    context->current_error = error;
}

void
Err_do_throw(Err *error) {
    ErrContext *context = Tls_get_err_context();

    if (context->current_env) {
        context->thrown_error = error;
        longjmp(*context->current_env, 1);
    }
    else {
        String *message = Err_Get_Mess(error);
        char *utf8 = Str_To_Utf8(message);
        fprintf(stderr, "%s", utf8);
        FREEMEM(utf8);
        exit(EXIT_FAILURE);
    }
}

void
Err_throw_mess(Class *klass, String *message) {
    UNUSED_VAR(klass);
    Err *err = Err_new(message);
    Err_do_throw(err);
}

void
Err_warn_mess(String *message) {
    char *utf8 = Str_To_Utf8(message);
    fprintf(stderr, "%s", utf8);
    FREEMEM(utf8);
    DECREF(message);
}

Err*
Err_trap(Err_Attempt_t routine, void *routine_context) {
    ErrContext *err_context = Tls_get_err_context();

    jmp_buf  env;
    jmp_buf *prev_env = err_context->current_env;
    err_context->current_env = &env;

    if (!setjmp(env)) {
        routine(routine_context);
    }

    err_context->current_env = prev_env;

    Err *error = err_context->thrown_error;
    err_context->thrown_error = NULL;
    return error;
}

/**** TestUtils ************************************************************/

void*
cfish_TestUtils_clone_host_runtime() {
    return NULL;
}

void
cfish_TestUtils_set_host_runtime(void *runtime) {
    UNUSED_VAR(runtime);
}

void
cfish_TestUtils_destroy_host_runtime(void *runtime) {
    UNUSED_VAR(runtime);
}

/**** To_Host methods ******************************************************/

void*
Str_To_Host_IMP(String *self) {
    Str_To_Host_t super_to_host
        = SUPER_METHOD_PTR(STRING, CFISH_Str_To_Host);
    return super_to_host(self);
}

void*
Blob_To_Host_IMP(Blob *self) {
    Blob_To_Host_t super_to_host
        = SUPER_METHOD_PTR(BLOB, CFISH_Blob_To_Host);
    return super_to_host(self);
}

void*
BB_To_Host_IMP(ByteBuf *self) {
    BB_To_Host_t super_to_host
        = SUPER_METHOD_PTR(BYTEBUF, CFISH_BB_To_Host);
    return super_to_host(self);
}

void*
Vec_To_Host_IMP(Vector *self) {
    Vec_To_Host_t super_to_host
        = SUPER_METHOD_PTR(VECTOR, CFISH_Vec_To_Host);
    return super_to_host(self);
}

void*
Hash_To_Host_IMP(Hash *self) {
    Hash_To_Host_t super_to_host
        = SUPER_METHOD_PTR(HASH, CFISH_Hash_To_Host);
    return super_to_host(self);
}

void*
Float32_To_Host_IMP(Float32 *self) {
    Float32_To_Host_t super_to_host
        = SUPER_METHOD_PTR(FLOAT32, CFISH_Float32_To_Host);
    return super_to_host(self);
}

void*
Float64_To_Host_IMP(Float64 *self) {
    Float64_To_Host_t super_to_host
        = SUPER_METHOD_PTR(FLOAT64, CFISH_Float64_To_Host);
    return super_to_host(self);
}

void*
Int32_To_Host_IMP(Integer32 *self) {
    Int32_To_Host_t super_to_host
        = SUPER_METHOD_PTR(INTEGER32, CFISH_Int32_To_Host);
    return super_to_host(self);
}

void*
Int64_To_Host_IMP(Integer64 *self) {
    Int64_To_Host_t super_to_host
        = SUPER_METHOD_PTR(INTEGER64, CFISH_Int64_To_Host);
    return super_to_host(self);
}

void*
Bool_To_Host_IMP(BoolNum *self) {
    Bool_To_Host_t super_to_host
        = SUPER_METHOD_PTR(BOOLNUM, CFISH_Bool_To_Host);
    return super_to_host(self);
}


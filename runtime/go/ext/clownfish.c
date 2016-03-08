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

#define CFISH_USE_SHORT_NAMES
#define C_CFISH_OBJ
#define C_CFISH_CLASS
#define C_CFISH_METHOD
#define C_CFISH_ERR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charmony.h"

#include "Clownfish/Blob.h"
#include "Clownfish/Boolean.h"
#include "Clownfish/ByteBuf.h"
#include "Clownfish/CharBuf.h"
#include "Clownfish/Class.h"
#include "Clownfish/Err.h"
#include "Clownfish/Hash.h"
#include "Clownfish/Method.h"
#include "Clownfish/Num.h"
#include "Clownfish/Obj.h"
#include "Clownfish/String.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Vector.h"

/* These symbols must be assigned real values during Go initialization,
 * which we'll confirm in Err_init().  */
void (*GoCfish_PanicErr)(Err *error);
Err* (*GoCfish_TrapErr)(Err_Attempt_t routine, void *context);

/******************************** Obj **************************************/

static CFISH_INLINE bool
SI_immortal(cfish_Class *klass) {
    if (klass == CFISH_CLASS
        || klass == CFISH_METHOD
        || klass == CFISH_BOOLEAN
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
Obj_To_Host_IMP(Obj *self, void *vcache) {
    UNUSED_VAR(self);
    UNUSED_VAR(vcache);
    THROW(ERR, "Unimplemented for Go");
    UNREACHABLE_RETURN(void*);
}

/******************************* Class *************************************/

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
    THROW(ERR, "Unimplemented for Go");
    UNREACHABLE_RETURN(String*);
}

void*
Class_To_Host_IMP(Class *self, void *vcache) {
    UNUSED_VAR(self);
    UNUSED_VAR(vcache);
    THROW(ERR, "Unimplemented for Go");
    UNREACHABLE_RETURN(void*);
}

/******************************* Method ************************************/

String*
Method_Host_Name_IMP(Method *self) {
    StringIterator *iter = StrIter_new(self->name, 0);
    CharBuf *charbuf = CB_new(Str_Get_Size(self->name));
    int32_t code_point;
    while (STR_OOB != (code_point = StrIter_Next(iter))) {
        if (code_point != '_') {
            CB_Cat_Char(charbuf, code_point);
        }
    }
    String *host_name = CB_Yield_String(charbuf);
    DECREF(charbuf);
    DECREF(iter);
    return host_name;
}

/******************************** Err **************************************/

/* TODO: Thread safety */
static Err *current_error;

void
Err_init_class(void) {
    if (GoCfish_PanicErr == NULL
        || GoCfish_TrapErr == NULL
       ) {
        fprintf(stderr, "Error at file %s line %d: Unexpected internal "
            "failure to initialize functions during bootstrapping\n",
            __FILE__, __LINE__);
        exit(1);
    }
}

Err*
Err_get_error() {
    return current_error;
}

void
Err_set_error(Err *error) {
    if (current_error) {
        DECREF(current_error);
    }
    current_error = error;
}

void
Err_do_throw(Err *error) {
    GoCfish_PanicErr(error);
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
Err_trap(Err_Attempt_t routine, void *context) {
    return GoCfish_TrapErr(routine, context);
}

/***************************** To_Host methods *****************************/

void*
Str_To_Host_IMP(String *self, void *vcache) {
    Str_To_Host_t super_to_host
        = SUPER_METHOD_PTR(STRING, CFISH_Str_To_Host);
    return super_to_host(self, vcache);
}

void*
Blob_To_Host_IMP(Blob *self, void *vcache) {
    Blob_To_Host_t super_to_host
        = SUPER_METHOD_PTR(BLOB, CFISH_Blob_To_Host);
    return super_to_host(self, vcache);
}

void*
BB_To_Host_IMP(ByteBuf *self, void *vcache) {
    BB_To_Host_t super_to_host
        = SUPER_METHOD_PTR(BYTEBUF, CFISH_BB_To_Host);
    return super_to_host(self, vcache);
}

void*
Vec_To_Host_IMP(Vector *self, void *vcache) {
    Vec_To_Host_t super_to_host
        = SUPER_METHOD_PTR(VECTOR, CFISH_Vec_To_Host);
    return super_to_host(self, vcache);
}

void*
Hash_To_Host_IMP(Hash *self, void *vcache) {
    Hash_To_Host_t super_to_host
        = SUPER_METHOD_PTR(HASH, CFISH_Hash_To_Host);
    return super_to_host(self, vcache);
}

void*
Float_To_Host_IMP(Float *self, void *vcache) {
    Float_To_Host_t super_to_host
        = SUPER_METHOD_PTR(FLOAT, CFISH_Float_To_Host);
    return super_to_host(self, vcache);
}

void*
Int_To_Host_IMP(Integer *self, void *vcache) {
    Int_To_Host_t super_to_host
        = SUPER_METHOD_PTR(INTEGER, CFISH_Int_To_Host);
    return super_to_host(self, vcache);
}

void*
Bool_To_Host_IMP(Boolean *self, void *vcache) {
    Bool_To_Host_t super_to_host
        = SUPER_METHOD_PTR(BOOLEAN, CFISH_Bool_To_Host);
    return super_to_host(self, vcache);
}



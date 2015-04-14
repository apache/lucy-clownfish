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
#define C_CFISH_LOCKFREEREGISTRY

#include <stdio.h>
#include <stdlib.h>

#include "charmony.h"

#include "Clownfish/Obj.h"
#include "Clownfish/Class.h"
#include "Clownfish/Method.h"
#include "Clownfish/Err.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/String.h"
#include "Clownfish/VArray.h"
#include "Clownfish/LockFreeRegistry.h"

/* These symbols must be assigned real values during Go initialization,
 * which we'll confirm in Err_init().  */
void (*GoCfish_PanicErr)(Err *error);
Err* (*GoCfish_TrapErr)(Err_Attempt_t routine, void *context);

/******************************** Obj **************************************/

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
    if (klass == CFISH_STRING || klass == CFISH_STACKSTRING) {
        return true;
    }
    return false;
}

static CFISH_INLINE bool
SI_threadsafe_but_not_immortal(cfish_Class *klass) {
    if (klass == CFISH_LOCKFREEREGISTRY) {
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
        else if (SI_threadsafe_but_not_immortal(klass)) {
            // TODO: use atomic operation
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
        else if (SI_threadsafe_but_not_immortal(klass)) {
            // TODO: use atomic operation
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
    THROW(ERR, "TODO");
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

/******************************* Method ************************************/

String*
Method_Host_Name_IMP(Method *self) {
    return (String*)INCREF(self->name);
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

void*
Err_To_Host_IMP(Err *self) {
    UNUSED_VAR(self);
    THROW(ERR, "TODO");
    UNREACHABLE_RETURN(void*);
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

/************************** LockFreeRegistry *******************************/

void*
LFReg_To_Host_IMP(LockFreeRegistry *self) {
    UNUSED_VAR(self);
    THROW(ERR, "TODO");
    UNREACHABLE_RETURN(void*);
}



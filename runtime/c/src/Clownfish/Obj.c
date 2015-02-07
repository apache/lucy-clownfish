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
#define CFISH_USE_SHORT_NAMES

#include "charmony.h"

#include "Clownfish/Obj.h"
#include "Clownfish/Err.h"
#include "Clownfish/String.h"

uint32_t
Obj_Get_RefCount_IMP(Obj *self) {
    return self->refcount;
}

static CFISH_INLINE bool
SI_immortal(cfish_Class *klass) {
    if (klass == CFISH_CLASS
        || klass == CFISH_METHOD
        || klass == CFISH_BOOLNUM
        || klass == CFISH_HASHTOMBSTONE
       ){
        return true;
    }
    return false;
}

static CFISH_INLINE bool
SI_is_string(cfish_Class *klass) {
    if (klass == CFISH_STRING || klass == CFISH_STACKSTRING) {
        return true;
    }
    return false;
}

Obj*
cfish_inc_refcount(void *vself) {
    Obj *self = (Obj*)vself;
    cfish_Class *const klass = self->klass;
    if (SI_immortal(klass)) {
        return self;
    }
    else if (SI_is_string(klass)
             && CFISH_Str_Is_Copy_On_IncRef((cfish_String*)self)
            ) {
        const char *utf8 = CFISH_Str_Get_Ptr8((cfish_String*)self);
        size_t size = CFISH_Str_Get_Size((cfish_String*)self);
        return (cfish_Obj*)cfish_Str_new_from_trusted_utf8(utf8, size);
    }
    else {
        self->refcount++;
        return self;
    }
}

Obj*
Obj_Inc_RefCount_IMP(Obj *self) {
    return cfish_inc_refcount(self);
}

uint32_t
cfish_dec_refcount(void *vself) {
    cfish_Obj *self = (Obj*)vself;
    cfish_Class *klass = self->klass;
    if (SI_immortal(klass)) {
        return self->refcount;
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

uint32_t
Obj_Dec_RefCount_IMP(Obj *self) {
    return cfish_dec_refcount(self);
}

void*
Obj_To_Host_IMP(Obj *self) {
    UNUSED_VAR(self);
    THROW(ERR, "TODO");
    UNREACHABLE_RETURN(void*);
}



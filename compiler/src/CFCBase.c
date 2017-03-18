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

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCUtil.h"

CFCBase*
CFCBase_allocate(const CFCMeta *meta) {
    CFCBase *self = (CFCBase*)CALLOCATE(meta->obj_alloc_size, 1);
    self->refcount = 1;
    self->weak_refcount = 0;
    self->meta = meta;
    return self;
}

void
CFCBase_destroy(CFCBase *self) {
    if (self->weak_refcount == 0) {
        FREEMEM(self);
    }
    else {
        // Let WeakPtr free the memory.
        self->refcount = 0;
    }
}

CFCBase*
CFCBase_incref(CFCBase *self) {
    if (self) {
        self->refcount++;
    }
    return self;
}

unsigned
CFCBase_decref(CFCBase *self) {
    if (!self) { return 0; }
    if (self->refcount > 1) {
        return --self->refcount;
    }
    else {
        // Don't decrease refcount to 0 before calling `destroy`. This could
        // make WeakPtrs free the object.
        self->meta->destroy(self);
        return 0;
    }
}

unsigned
CFCBase_get_refcount(CFCBase *self) {
    return self->refcount;
}

const char*
CFCBase_get_cfc_class(CFCBase *self) {
    return self->meta->cfc_class;
}

CFCWeakPtr
CFCWeakPtr_new(CFCBase *base) {
    CFCWeakPtr self = { base };
    if (base) { base->weak_refcount += 1; }
    return self;
}

CFCBase*
CFCWeakPtr_deref(CFCWeakPtr self) {
    if (self.ptr_ != NULL && self.ptr_->refcount == 0) {
        // CFC doesn't access WeakPtrs after the strong refcount went to
        // zero, so throw an exception.
        CFCUtil_die("Invalid WeakPtr deref");
    }
    return self.ptr_;
}

void
CFCWeakPtr_set(CFCWeakPtr *self, CFCBase *base) {
    CFCBase *old_base = self->ptr_;
    if (old_base == base) { return; }
    if (old_base != NULL) {
        old_base->weak_refcount -= 1;
        if (old_base->refcount == 0 && old_base->weak_refcount == 0) {
            FREEMEM(old_base);
        }
    }
    self->ptr_ = base;
    if (base) { base->weak_refcount += 1; }
}

void
CFCWeakPtr_destroy(CFCWeakPtr *self) {
    CFCBase *base = self->ptr_;
    if (base == NULL) { return; }
    base->weak_refcount -= 1;
    if (base->refcount == 0 && base->weak_refcount == 0) {
        FREEMEM(base);
    }
    self->ptr_ = NULL;
}



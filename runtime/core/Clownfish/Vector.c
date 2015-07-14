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

#define C_CFISH_VECTOR
#include <string.h>
#include <stdlib.h>

#define CFISH_USE_SHORT_NAMES

#include "Clownfish/Class.h"
#include "Clownfish/Vector.h"
#include "Clownfish/Err.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Util/SortUtils.h"

static CFISH_INLINE void
SI_copy_and_incref(Obj **dst, Obj **src, size_t num);

static CFISH_INLINE void
SI_add_grow_and_oversize(Vector *self, size_t addend1, size_t addend2);

static void
S_grow_and_oversize(Vector *self, size_t min_size);

static CFISH_INLINE void
SI_grow(Vector *self, size_t capacity);

static void
S_overflow_error(void);

Vector*
Vec_new(size_t capacity) {
    Vector *self = (Vector*)Class_Make_Obj(VECTOR);
    Vec_init(self, capacity);
    return self;
}

Vector*
Vec_init(Vector *self, size_t capacity) {
    // Init.
    self->size = 0;

    // Assign.
    self->cap = capacity;

    // Derive.
    self->elems = (Obj**)CALLOCATE(capacity, sizeof(Obj*));

    return self;
}

void
Vec_Destroy_IMP(Vector *self) {
    if (self->elems) {
        Obj **elems        = self->elems;
        Obj **const limit  = elems + self->size;
        for (; elems < limit; elems++) {
            DECREF(*elems);
        }
        FREEMEM(self->elems);
    }
    SUPER_DESTROY(self, VECTOR);
}

Vector*
Vec_Clone_IMP(Vector *self) {
    Vector *twin = Vec_new(self->size);
    twin->size = self->size;
    SI_copy_and_incref(twin->elems, self->elems, self->size);

    return twin;
}

void
Vec_Push_IMP(Vector *self, Obj *element) {
    SI_add_grow_and_oversize(self, self->size, 1);
    self->elems[self->size] = element;
    self->size++;
}

void
Vec_Push_All_IMP(Vector *self, Vector *other) {
    SI_add_grow_and_oversize(self, self->size, other->size);
    SI_copy_and_incref(self->elems + self->size, other->elems, other->size);
    self->size += other->size;
}

Obj*
Vec_Pop_IMP(Vector *self) {
    if (!self->size) {
        return NULL;
    }
    self->size--;
    return  self->elems[self->size];
}

void
Vec_Insert_IMP(Vector *self, size_t tick, Obj *elem) {
    if (tick >= self->size) {
        Vec_Store(self, tick, elem);
        return;
    }

    SI_add_grow_and_oversize(self, self->size, 1);
    memmove(self->elems + tick + 1, self->elems + tick,
            (self->size - tick) * sizeof(Obj*));
    self->elems[tick] = elem;
    self->size++;
}

void
Vec_Insert_All_IMP(Vector *self, size_t tick, Vector *other) {
    SI_add_grow_and_oversize(self, tick, other->size);

    if (tick < self->size) {
        memmove(self->elems + tick + other->size, self->elems + tick,
                (self->size - tick) * sizeof(Obj*));
    }
    else {
        memset(self->elems + self->size, 0,
               (tick - self->size) * sizeof(Obj*));
    }

    SI_copy_and_incref(self->elems + tick, other->elems, other->size);
    self->size = tick + other->size;
}

Obj*
Vec_Fetch_IMP(Vector *self, size_t num) {
    if (num >= self->size) {
        return NULL;
    }

    return self->elems[num];
}

void
Vec_Store_IMP(Vector *self, size_t tick, Obj *elem) {
    SI_add_grow_and_oversize(self, tick, 1);
    if (tick < self->size) {
        DECREF(self->elems[tick]);
    }
    else {
        memset(self->elems + self->size, 0,
               (tick - self->size) * sizeof(Obj*));
        self->size = tick + 1;
    }
    self->elems[tick] = elem;
}

void
Vec_Grow_IMP(Vector *self, size_t capacity) {
    if (capacity > self->cap) {
        SI_grow(self, capacity);
    }
}

Obj*
Vec_Delete_IMP(Vector *self, size_t num) {
    Obj *elem = NULL;
    if (num < self->size) {
        elem = self->elems[num];
        self->elems[num] = NULL;
    }
    return elem;
}

void
Vec_Excise_IMP(Vector *self, size_t offset, size_t length) {
    if (offset >= self->size)         { return; }
    if (length > self->size - offset) { length = self->size - offset; }

    Obj **elems = self->elems;
    for (size_t i = 0; i < length; i++) {
        DECREF(elems[offset + i]);
    }

    size_t num_to_move = self->size - (offset + length);
    memmove(self->elems + offset, self->elems + offset + length,
            num_to_move * sizeof(Obj*));
    self->size -= length;
}

void
Vec_Clear_IMP(Vector *self) {
    Vec_Excise_IMP(self, 0, self->size);
}

void
Vec_Resize_IMP(Vector *self, size_t size) {
    if (size < self->size) {
        Vec_Excise(self, size, self->size - size);
    }
    else if (size > self->size) {
        Vec_Grow(self, size);
        memset(self->elems + self->size, 0,
               (size - self->size) * sizeof(Obj*));
    }
    self->size = size;
}

size_t
Vec_Get_Size_IMP(Vector *self) {
    return self->size;
}

size_t
Vec_Get_Capacity_IMP(Vector *self) {
    return self->cap;
}

static int
S_default_compare(void *context, const void *va, const void *vb) {
    Obj *a = *(Obj**)va;
    Obj *b = *(Obj**)vb;
    UNUSED_VAR(context);
    if (a != NULL && b != NULL)      { return Obj_Compare_To(a, b); }
    else if (a == NULL && b == NULL) { return 0;  }
    else if (a == NULL)              { return 1;  } // NULL to the back
    else  /* b == NULL */            { return -1; } // NULL to the back
}

void
Vec_Sort_IMP(Vector *self) {
    void *scratch = MALLOCATE(self->size * sizeof(Obj*));
    Sort_mergesort(self->elems, scratch, self->size, sizeof(void*),
                   S_default_compare, NULL);
    FREEMEM(scratch);
}

bool
Vec_Equals_IMP(Vector *self, Obj *other) {
    Vector *twin = (Vector*)other;
    if (twin == self)             { return true; }
    if (!Obj_is_a(other, VECTOR)) { return false; }
    if (twin->size != self->size) {
        return false;
    }
    else {
        Obj **elems      = self->elems;
        Obj **twin_elems = twin->elems;
        for (size_t i = 0, max = self->size; i < max; i++) {
            Obj *val       = elems[i];
            Obj *other_val = twin_elems[i];
            if ((val && !other_val) || (other_val && !val)) { return false; }
            if (val && !Obj_Equals(val, other_val))         { return false; }
        }
    }
    return true;
}

Vector*
Vec_Slice_IMP(Vector *self, size_t offset, size_t length) {
    // Adjust ranges if necessary.
    if (offset >= self->size) {
        offset = 0;
        length = 0;
    }
    else if (length > self->size - offset) {
        length = self->size - offset;
    }

    // Copy elements.
    Vector *slice = Vec_new(length);
    slice->size = length;
    SI_copy_and_incref(slice->elems, self->elems + offset, length);

    return slice;
}

static CFISH_INLINE void
SI_copy_and_incref(Obj **dst, Obj **src, size_t num) {
    for (size_t i = 0; i < num; i++) {
        dst[i] = INCREF(src[i]);
    }
}

// Ensure that the vector's capacity is at least (addend1 + addend2).
// If the vector must be grown, oversize the allocation.
static CFISH_INLINE void
SI_add_grow_and_oversize(Vector *self, size_t addend1, size_t addend2) {
    size_t min_size = addend1 + addend2;
    // Check for overflow.
    if (min_size < addend1) {
        S_overflow_error();
        return;
    }

    if (min_size > self->cap) {
        S_grow_and_oversize(self, min_size);
    }
}

static void
S_grow_and_oversize(Vector *self, size_t min_size) {
    // Oversize by 25%, but at least four elements.
    size_t extra = min_size / 4;
    if (extra < 4) { extra = 4; }

    size_t capacity = min_size + extra;
    // Check for overflow.
    if (capacity < min_size) {
        S_overflow_error();
        return;
    }

    SI_grow(self, capacity);
}

static CFISH_INLINE void
SI_grow(Vector *self, size_t capacity) {
    if (capacity > SIZE_MAX / sizeof(Obj*)) {
        S_overflow_error();
        return;
    }
    self->elems = (Obj**)REALLOCATE(self->elems, capacity * sizeof(Obj*));
    self->cap   = capacity;
}

static void
S_overflow_error() {
    THROW(ERR, "Vector index overflow");
}


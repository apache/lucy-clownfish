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

#define C_CFISH_VARRAY
#include <string.h>
#include <stdlib.h>

#define CFISH_USE_SHORT_NAMES

#include "Clownfish/Class.h"
#include "Clownfish/VArray.h"
#include "Clownfish/Err.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Util/SortUtils.h"

static CFISH_INLINE void
SI_grow_by(VArray *self, size_t add_size);

VArray*
VA_new(size_t capacity) {
    VArray *self = (VArray*)Class_Make_Obj(VARRAY);
    VA_init(self, capacity);
    return self;
}

VArray*
VA_init(VArray *self, size_t capacity) {
    // Init.
    self->size = 0;

    // Assign.
    self->cap = capacity;

    // Derive.
    self->elems = (Obj**)CALLOCATE(capacity, sizeof(Obj*));

    return self;
}

void
VA_Destroy_IMP(VArray *self) {
    if (self->elems) {
        Obj **elems        = self->elems;
        Obj **const limit  = elems + self->size;
        for (; elems < limit; elems++) {
            DECREF(*elems);
        }
        FREEMEM(self->elems);
    }
    SUPER_DESTROY(self, VARRAY);
}

VArray*
VA_Clone_IMP(VArray *self) {
    VArray *twin = VA_new(self->size);

    // Clone each element.
    for (size_t i = 0; i < self->size; i++) {
        Obj *elem = self->elems[i];
        if (elem) {
            twin->elems[i] = Obj_Clone(elem);
        }
    }

    // Ensure that size is the same if NULL elems at end.
    twin->size = self->size;

    return twin;
}

VArray*
VA_Shallow_Copy_IMP(VArray *self) {
    // Dupe, then increment refcounts.
    VArray *twin = VA_new(self->size);
    Obj **elems = twin->elems;
    memcpy(elems, self->elems, self->size * sizeof(Obj*));
    twin->size = self->size;
    for (size_t i = 0; i < self->size; i++) {
        if (elems[i] != NULL) {
            (void)INCREF(elems[i]);
        }
    }

    return twin;
}

void
VA_Push_IMP(VArray *self, Obj *element) {
    if (self->size == self->cap) {
        SI_grow_by(self, 1);
    }
    self->elems[self->size] = element;
    self->size++;
}

void
VA_Push_VArray_IMP(VArray *self, VArray *other) {
    if (other->size > self->cap - self->size) {
        SI_grow_by(self, other->size);
    }
    for (size_t i = 0, tick = self->size; i < other->size; i++, tick++) {
        Obj *elem = VA_Fetch(other, i);
        if (elem != NULL) {
            self->elems[tick] = INCREF(elem);
        }
    }
    self->size += other->size;
}

Obj*
VA_Pop_IMP(VArray *self) {
    if (!self->size) {
        return NULL;
    }
    self->size--;
    return  self->elems[self->size];
}

void
VA_Unshift_IMP(VArray *self, Obj *elem) {
    if (self->size == self->cap) {
        SI_grow_by(self, 1);
    }
    memmove(self->elems + 1, self->elems, self->size * sizeof(Obj*));
    self->elems[0] = elem;
    self->size++;
}

Obj*
VA_Shift_IMP(VArray *self) {
    if (!self->size) {
        return NULL;
    }
    else {
        Obj *const return_val = self->elems[0];
        self->size--;
        if (self->size > 0) {
            memmove(self->elems, self->elems + 1,
                    self->size * sizeof(Obj*));
        }
        return return_val;
    }
}

Obj*
VA_Fetch_IMP(VArray *self, size_t num) {
    if (num >= self->size) {
        return NULL;
    }

    return self->elems[num];
}

void
VA_Store_IMP(VArray *self, size_t tick, Obj *elem) {
    if (tick >= self->cap) {
        if (tick == SIZE_MAX) {
            THROW(ERR, "Invalid tick");
        }
        SI_grow_by(self, tick + 1 - self->size);
    }
    if (tick < self->size) { DECREF(self->elems[tick]); }
    else                   { self->size = tick + 1; }
    self->elems[tick] = elem;
}

void
VA_Grow_IMP(VArray *self, size_t capacity) {
    if (capacity > self->cap) {
        self->elems = (Obj**)REALLOCATE(self->elems, capacity * sizeof(Obj*));
        self->cap   = capacity;
        memset(self->elems + self->size, 0,
               (capacity - self->size) * sizeof(Obj*));
    }
}

Obj*
VA_Delete_IMP(VArray *self, size_t num) {
    Obj *elem = NULL;
    if (num < self->size) {
        elem = self->elems[num];
        self->elems[num] = NULL;
    }
    return elem;
}

void
VA_Excise_IMP(VArray *self, size_t offset, size_t length) {
    if (offset >= self->size)         { return; }
    if (length > self->size - offset) { length = self->size - offset; }

    for (size_t i = 0; i < length; i++) {
        DECREF(self->elems[offset + i]);
    }

    size_t num_to_move = self->size - (offset + length);
    memmove(self->elems + offset, self->elems + offset + length,
            num_to_move * sizeof(Obj*));
    self->size -= length;
}

void
VA_Clear_IMP(VArray *self) {
    VA_Excise_IMP(self, 0, self->size);
}

void
VA_Resize_IMP(VArray *self, size_t size) {
    if (size < self->size) {
        VA_Excise(self, size, self->size - size);
    }
    else if (size > self->size) {
        VA_Grow(self, size);
    }
    self->size = size;
}

size_t
VA_Get_Size_IMP(VArray *self) {
    return self->size;
}

size_t
VA_Get_Capacity_IMP(VArray *self) {
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
VA_Sort_IMP(VArray *self, CFISH_Sort_Compare_t compare, void *context) {
    if (!compare) { compare = S_default_compare; }
    Sort_quicksort(self->elems, self->size, sizeof(void*), compare, context);
}

bool
VA_Equals_IMP(VArray *self, Obj *other) {
    VArray *twin = (VArray*)other;
    if (twin == self)             { return true; }
    if (!Obj_Is_A(other, VARRAY)) { return false; }
    if (twin->size != self->size) {
        return false;
    }
    else {
        for (size_t i = 0, max = self->size; i < max; i++) {
            Obj *val       = self->elems[i];
            Obj *other_val = twin->elems[i];
            if ((val && !other_val) || (other_val && !val)) { return false; }
            if (val && !Obj_Equals(val, other_val))         { return false; }
        }
    }
    return true;
}

VArray*
VA_Gather_IMP(VArray *self, VA_Gather_Test_t test, void *data) {
    VArray *gathered = VA_new(self->size);
    for (size_t i = 0, max = self->size; i < max; i++) {
        if (test(self, i, data)) {
            Obj *elem = self->elems[i];
            VA_Push(gathered, elem ? INCREF(elem) : NULL);
        }
    }
    return gathered;
}

VArray*
VA_Slice_IMP(VArray *self, size_t offset, size_t length) {
    // Adjust ranges if necessary.
    if (offset >= self->size) {
        offset = 0;
        length = 0;
    }
    else if (length > self->size - offset) {
        length = self->size - offset;
    }

    // Copy elements.
    VArray *slice = VA_new(length);
    slice->size = length;
    Obj **slice_elems = slice->elems;
    Obj **my_elems    = self->elems;
    for (size_t i = 0; i < length; i++) {
        slice_elems[i] = INCREF(my_elems[offset + i]);
    }

    return slice;
}

static void
SI_grow_by(VArray *self, size_t add_size) {
    size_t min_size = self->size + add_size;
    // Check for overflow.
    if (min_size < add_size) {
        THROW(ERR, "Array grew too large");
    }
    size_t new_size = Memory_oversize(min_size, sizeof(Obj*));
    VA_Grow(self, new_size);
}


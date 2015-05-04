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

#define C_CFISH_BLOB
#define CFISH_USE_SHORT_NAMES

#include <string.h>

#include "Clownfish/Class.h"
#include "Clownfish/Blob.h"
#include "Clownfish/Err.h"
#include "Clownfish/Util/Memory.h"

Blob*
Blob_new(const char *buf, size_t size) {
    Blob *self = (Blob*)Class_Make_Obj(BLOB);
    return Blob_init(self, buf, size);
}

Blob*
Blob_init(Blob *self, const char *buf, size_t size) {
    char *copy = (char*)MALLOCATE(size);
    memcpy(copy, buf, size);

    self->buf      = copy;
    self->size     = size;
    self->owns_buf = true;

    return self;
}

Blob*
Blob_new_steal(char *buf, size_t size) {
    Blob *self = (Blob*)Class_Make_Obj(BLOB);
    return Blob_init_steal(self, buf, size);
}

Blob*
Blob_init_steal(Blob *self, char *buf, size_t size) {
    self->buf      = buf;
    self->size     = size;
    self->owns_buf = true;

    return self;
}

Blob*
Blob_new_wrap(const char *buf, size_t size) {
    Blob *self = (Blob*)Class_Make_Obj(BLOB);
    return Blob_init_wrap(self, buf, size);
}

Blob*
Blob_init_wrap(Blob *self, const char *buf, size_t size) {
    self->buf      = buf;
    self->size     = size;
    self->owns_buf = false;

    return self;
}

void
Blob_Destroy_IMP(Blob *self) {
    if (self->owns_buf) { FREEMEM((char*)self->buf); }
    SUPER_DESTROY(self, BLOB);
}

Blob*
Blob_Clone_IMP(Blob *self) {
    return (Blob*)INCREF(self);
}

const char*
Blob_Get_Buf_IMP(Blob *self) {
    return self->buf;
}

size_t
Blob_Get_Size_IMP(Blob *self) {
    return self->size;
}

static CFISH_INLINE bool
SI_equals_bytes(Blob *self, const void *bytes, size_t size) {
    if (self->size != size) { return false; }
    return (memcmp(self->buf, bytes, self->size) == 0);
}

bool
Blob_Equals_IMP(Blob *self, Obj *other) {
    Blob *const twin = (Blob*)other;
    if (twin == self)           { return true; }
    if (!Obj_Is_A(other, BLOB)) { return false; }
    return SI_equals_bytes(self, twin->buf, twin->size);
}

bool
Blob_Equals_Bytes_IMP(Blob *self, const void *bytes, size_t size) {
    return SI_equals_bytes(self, bytes, size);
}

int
Blob_compare(const void *va, const void *vb) {
    Blob *a = *(Blob**)va;
    Blob *b = *(Blob**)vb;
    const size_t size = a->size < b->size ? a->size : b->size;

    int32_t comparison = memcmp(a->buf, b->buf, size);

    if (comparison == 0 && a->size != b->size) {
        comparison = a->size < b->size ? -1 : 1;
    }

    return comparison;
}

int32_t
Blob_Compare_To_IMP(Blob *self, Obj *other) {
    CERTIFY(other, BLOB);
    return Blob_compare(&self, &other);
}



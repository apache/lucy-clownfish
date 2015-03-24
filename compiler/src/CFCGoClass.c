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

#include <string.h>
#include <stdlib.h>

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCGoClass.h"
#include "CFCUtil.h"
#include "CFCClass.h"
#include "CFCMethod.h"
#include "CFCParcel.h"
#include "CFCParamList.h"
#include "CFCFunction.h"
#include "CFCSymbol.h"
#include "CFCVariable.h"
#include "CFCType.h"
#include "CFCGoTypeMap.h"

struct CFCGoClass {
    CFCBase base;
    CFCParcel *parcel;
    char *class_name;
    CFCClass *client;
};

static CFCGoClass **registry = NULL;
static size_t registry_size = 0;
static size_t registry_cap  = 0;

static void
S_CFCGoClass_destroy(CFCGoClass *self);

static const CFCMeta CFCGOCLASS_META = {
    "Clownfish::CFC::Binding::Go::Class",
    sizeof(CFCGoClass),
    (CFCBase_destroy_t)S_CFCGoClass_destroy
};

CFCGoClass*
CFCGoClass_new(CFCParcel *parcel, const char *class_name) {
    CFCUTIL_NULL_CHECK(parcel);
    CFCUTIL_NULL_CHECK(class_name);
    CFCGoClass *self = (CFCGoClass*)CFCBase_allocate(&CFCGOCLASS_META);
    self->parcel = (CFCParcel*)CFCBase_incref((CFCBase*)parcel);
    self->class_name = CFCUtil_strdup(class_name);
    // Client may be NULL, since fetch_singleton() does not always succeed.
    CFCClass *client = CFCClass_fetch_singleton(parcel, class_name);
    self->client = (CFCClass*)CFCBase_incref((CFCBase*)client);
    return self;
}

static void
S_CFCGoClass_destroy(CFCGoClass *self) {
    CFCBase_decref((CFCBase*)self->parcel);
    CFCBase_decref((CFCBase*)self->client);
    FREEMEM(self->class_name);
    CFCBase_destroy((CFCBase*)self);
}

static int
S_compare_cfcgoclass(const void *va, const void *vb) {
    CFCGoClass *a = *(CFCGoClass**)va;
    CFCGoClass *b = *(CFCGoClass**)vb;
    return strcmp(a->class_name, b->class_name);
}

void
CFCGoClass_register(CFCGoClass *self) {
    if (registry_size == registry_cap) {
        size_t new_cap = registry_cap + 10;
        size_t amount = (new_cap + 1) * sizeof(CFCGoClass*);
        registry = (CFCGoClass**)REALLOCATE(registry, amount);
        for (size_t i = registry_cap; i <= new_cap; i++) {
            registry[i] = NULL;
        }
        registry_cap = new_cap;
    }
    CFCGoClass *existing = CFCGoClass_singleton(self->class_name);
    if (existing) {
        CFCUtil_die("Class '%s' already registered", self->class_name);
    }
    registry[registry_size] = (CFCGoClass*)CFCBase_incref((CFCBase*)self);
    registry_size++;
    qsort(registry, registry_size, sizeof(CFCGoClass*),
          S_compare_cfcgoclass);
}

CFCGoClass*
CFCGoClass_singleton(const char *class_name) {
    CFCUTIL_NULL_CHECK(class_name);
    for (size_t i = 0; i < registry_size; i++) {
        CFCGoClass *existing = registry[i];
        if (strcmp(class_name, existing->class_name) == 0) {
            return existing;
        }
    }
    return NULL;
}

CFCGoClass**
CFCGoClass_registry() {
    if (!registry) {
        registry = (CFCGoClass**)CALLOCATE(1, sizeof(CFCGoClass*));
    }
    return registry;
}

void
CFCGoClass_clear_registry(void) {
    for (size_t i = 0; i < registry_size; i++) {
        CFCBase_decref((CFCBase*)registry[i]);
    }
    FREEMEM(registry);
    registry_size = 0;
    registry_cap  = 0;
    registry      = NULL;
}


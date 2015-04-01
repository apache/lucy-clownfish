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
#include "CFCGoFunc.h"
#include "CFCGoMethod.h"
#include "CFCGoTypeMap.h"

struct CFCGoClass {
    CFCBase base;
    CFCParcel *parcel;
    char *class_name;
    CFCClass *client;
    CFCGoMethod **method_bindings;
    size_t num_bound;
};

static CFCGoClass **registry = NULL;
static size_t registry_size = 0;
static size_t registry_cap  = 0;

static void
S_CFCGoClass_destroy(CFCGoClass *self);

static void
S_lazy_init_method_bindings(CFCGoClass *self);

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
    for (int i = 0; self->method_bindings[i] != NULL; i++) {
        CFCBase_decref((CFCBase*)self->method_bindings[i]);
    }
    FREEMEM(self->method_bindings);
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

CFCClass*
CFCGoClass_get_client(CFCGoClass *self) {
    return self->client;
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

char*
CFCGoClass_go_typing(CFCGoClass *self) {
    char *content = NULL;
    if (!self->client) {
        CFCUtil_die("Can't find class for %s", self->class_name);
    }
    else if (CFCClass_inert(self->client)) {
        content = CFCUtil_strdup("");
    } else {
        const char *short_struct = CFCClass_get_struct_sym(self->client);
        const char *full_struct  = CFCClass_full_struct_sym(self->client);

        CFCClass *parent = CFCClass_get_parent(self->client);
        char *parent_iface;
        if (parent) {
            const char *parent_struct = CFCClass_get_struct_sym(parent);
            CFCParcel *parent_parcel = CFCClass_get_parcel(parent);
            if (parent_parcel == self->parcel) {
                parent_iface = CFCUtil_sprintf("\t%s\n", parent_struct);
            }
            else {
                char *parent_package
                    = CFCGoTypeMap_go_short_package(parent_parcel);
                parent_iface = CFCUtil_sprintf("\t%s.%s\n", parent_package,
                                               parent_struct);
            }
        }
        else {
            parent_iface = CFCUtil_strdup("");
        }

        // Temporary hack until it's possible to customize interfaces.
        char *temp_hack_iface_methods;
        if (strcmp(self->class_name, "Clownfish::Obj") == 0) {
            temp_hack_iface_methods = CFCUtil_strdup("\tTOPTR() uintptr\n");
        }
        else if (strcmp(self->class_name, "Clownfish::Err") == 0) {
            temp_hack_iface_methods = CFCUtil_strdup("\tError() string\n");
        }
        else {
            temp_hack_iface_methods = CFCUtil_strdup("");
        }

        char pattern[] =
            "type %s interface {\n"
            "%s"
            "%s"
            "}\n"
            "\n"
            "type impl%s struct {\n"
            "\tref *C.%s\n"
            "}\n"
            ;
        content = CFCUtil_sprintf(pattern, short_struct, parent_iface,
                                  temp_hack_iface_methods,
                                  short_struct, full_struct);
        FREEMEM(temp_hack_iface_methods);
        FREEMEM(parent_iface);
    }
    return content;
}

char*
CFCGoClass_boilerplate_funcs(CFCGoClass *self) {
    char *content = NULL;
    if (!self->client) {
        CFCUtil_die("Can't find class for %s", self->class_name);
    }
    else if (CFCClass_inert(self->client)) {
        content = CFCUtil_strdup("");
    } else {
        const char *short_struct = CFCClass_get_struct_sym(self->client);
        const char *full_struct  = CFCClass_full_struct_sym(self->client);
        char pattern[] =
            "func WRAP%s(ptr unsafe.Pointer) %s {\n"
            "\tobj := &impl%s{((*C.%s)(ptr))}\n"
            "\truntime.SetFinalizer(obj, (*impl%s).finalize)\n"
            "\treturn obj\n"
            "}\n"
            "\n"
            "func (obj *impl%s) finalize() {\n"
            "\tC.cfish_dec_refcount(unsafe.Pointer(obj.ref))\n"
            "\tobj.ref = nil\n"
            "}\n"
            "\n"
            "func (obj *impl%s) TOPTR() uintptr {\n"
            "\treturn uintptr(unsafe.Pointer(obj.ref))\n"
            "}\n"
            ;

        content = CFCUtil_sprintf(pattern, short_struct, short_struct,
                                  short_struct, full_struct, short_struct,
                                  short_struct, short_struct);
    }
    return content;
}

static void
S_lazy_init_method_bindings(CFCGoClass *self) {
    if (self->method_bindings) {
        return;
    }
    CFCUTIL_NULL_CHECK(self->client);
    CFCClass     *parent        = CFCClass_get_parent(self->client);
    size_t        num_bound     = 0;
    CFCMethod   **fresh_methods = CFCClass_fresh_methods(self->client);
    CFCGoMethod **bound
        = (CFCGoMethod**)CALLOCATE(1, sizeof(CFCGoMethod*));

     // Iterate over the class's fresh methods.
    for (size_t i = 0; fresh_methods[i] != NULL; i++) {
        CFCMethod *method = fresh_methods[i];

        // Skip methods which have been explicitly excluded.
        if (CFCMethod_excluded_from_host(method)) {
            continue;
        }

        // Skip methods that shouldn't be bound.
        if (!CFCMethod_can_be_bound(method)) {
            continue;
        }

        /* Create the binding, add it to the array.
         */
        CFCGoMethod *meth_binding = CFCGoMethod_new(method);
        size_t size = (num_bound + 2) * sizeof(CFCGoMethod*);
        bound = (CFCGoMethod**)REALLOCATE(bound, size);
        bound[num_bound] = meth_binding;
        num_bound++;
        bound[num_bound] = NULL;
    }

    self->method_bindings = bound;
    self->num_bound       = num_bound;
}


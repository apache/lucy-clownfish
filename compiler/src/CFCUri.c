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

#include <stdlib.h>
#include <string.h>

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCUri.h"
#include "CFCClass.h"
#include "CFCDocument.h"
#include "CFCFunction.h"
#include "CFCMethod.h"
#include "CFCParcel.h"
#include "CFCUtil.h"

struct CFCUri {
    CFCBase      base;
    char        *string;
    CFCClass    *base_class;
    CFCUriType   type;
    CFCClass    *klass;
    CFCDocument *document;
    char        *callable;
    char        *error;
};

static const CFCMeta CFCURI_META = {
    "Clownfish::CFC::Uri",
    sizeof(CFCUri),
    (CFCBase_destroy_t)CFCUri_destroy
};

static void
S_parse(CFCUri *self);

static void
S_resolve(CFCUri *self, const char *prefix, const char *struct_sym,
          const char *callable);

static CFCClass*
S_find_class_full(CFCUri *self, const char *parcel, const char *struct_sym);

static CFCClass*
S_find_class_short(CFCUri *self, const char *struct_sym);

static void
S_set_error(CFCUri *self, const char *error);

static char*
S_next_component(char **iter);

int
CFCUri_is_clownfish_uri(const char *uri) {
    return strncmp(uri, "cfish:", 6) == 0 || !strchr(uri, ':');
}

CFCUri*
CFCUri_new(const char *uri, CFCClass *base_class) {
    CFCUri *self = (CFCUri*)CFCBase_allocate(&CFCURI_META);
    return CFCUri_init(self, uri, base_class);
}

CFCUri*
CFCUri_init(CFCUri *self, const char *uri, CFCClass *base_class) {
    CFCUTIL_NULL_CHECK(uri);

    self->string     = CFCUtil_strdup(uri);
    self->base_class = (CFCClass*)CFCBase_incref((CFCBase*)base_class);

    return self;
}

void
CFCUri_destroy(CFCUri *self) {
    FREEMEM(self->string);
    FREEMEM(self->callable);
    FREEMEM(self->error);
    CFCBase_decref((CFCBase*)self->base_class);
    CFCBase_decref((CFCBase*)self->klass);
    CFCBase_decref((CFCBase*)self->document);
    CFCBase_destroy((CFCBase*)self);
}

static void
S_parse(CFCUri *self) {
    const char *uri_part = self->string;

    if (strncmp(uri_part, "cfish:", 6) == 0) {
        uri_part += 6;
    }

    if (strcmp(uri_part, "@null") == 0) {
        self->type = CFC_URI_NULL;
        return;
    }

    const char *parcel     = NULL;
    const char *struct_sym = NULL;
    const char *callable   = NULL;

    char       *buf  = CFCUtil_strdup(uri_part);
    char       *iter = buf;
    const char *component = S_next_component(&iter);

    if (CFCUtil_islower(component[0])) {
        // Parcel
        parcel = component;
        component = S_next_component(&iter);

        if (!component) {
            S_set_error(self, "Missing component in Clownfish URI");
            goto done;
        }
    }

    // struct_sym == NULL for "cfish:.Method" style URL.
    // parcel implies struct_sym.
    if (parcel || component[0] != '\0') {
        struct_sym = component;
    }

    component = S_next_component(&iter);

    if (component) {
        callable = component;
        component = S_next_component(&iter);
    }

    if (component) {
        S_set_error(self, "Trailing component in Clownfish URI");
        goto done;
    }

    S_resolve(self, parcel, struct_sym, callable);

done:
    FREEMEM(buf);
}

static void
S_resolve(CFCUri *self, const char *parcel, const char *struct_sym,
          const char *callable) {
    // Try to find a CFCClass.
    CFCClass *klass     = NULL;

    if (parcel) {
        klass = S_find_class_full(self, parcel, struct_sym);
    }
    else if (struct_sym) {
        klass = S_find_class_short(self, struct_sym);
    }
    else if (callable) {
        klass = self->base_class;
    }

    if (klass) {
        if (!CFCClass_public(klass)) {
            CFCUtil_warn("Non-public class '%s' in Clownfish URI: %s",
                          CFCClass_get_struct_sym(klass), self->string);
        }

        self->type  = CFC_URI_CLASS;
        self->klass = klass;
        CFCBase_incref((CFCBase*)klass);

        if (callable) {
            if (CFCUtil_islower(callable[0])) {
                CFCFunction *function = CFCClass_function(klass, callable);

                if (!function) {
                    CFCUtil_warn("Unknown function '%s' in Clownfish URI: %s",
                                 callable, self->string);
                }
                else if (!CFCFunction_public(function)) {
                    CFCUtil_warn("Non-public function '%s' in Clownfish URI:"
                                 " %s", callable, self->string);
                }

                self->type     = CFC_URI_FUNCTION;
                self->callable = CFCUtil_strdup(callable);
            }
            else {
                CFCMethod *method = CFCClass_method(klass, callable);

                if (!method) {
                    CFCUtil_warn("Unknown method '%s' in Clownfish URI: %s",
                                 callable, self->string);
                }
                else if (!CFCMethod_public(method)) {
                    CFCUtil_warn("Non-public method '%s' in Clownfish URI:"
                                 " %s", callable, self->string);
                }

                self->type     = CFC_URI_METHOD;
                self->callable = CFCUtil_strdup(callable);
            }
        }

        return;
    }

    // Try to find a CFCDocument.
    if (!parcel && struct_sym && !callable) {
        CFCDocument *doc = CFCDocument_fetch(struct_sym);

        if (doc) {
            self->type     = CFC_URI_DOCUMENT;
            self->document = doc;
            CFCBase_incref((CFCBase*)doc);
            return;
        }
    }

    S_set_error(self, "Couldn't resolve Clownfish URI");
}

static CFCClass*
S_find_class_full(CFCUri *self, const char *parcel, const char *struct_sym) {
    char *full_struct_sym = CFCUtil_sprintf("%s_%s", parcel, struct_sym);
    CFCClass *klass = NULL;

    if (self->base_class) {
        // Only look in parcel of base class.
        CFCParcel *parcel = CFCClass_get_parcel(self->base_class);
        klass = CFCParcel_class_by_full_sym(parcel, full_struct_sym);
    }
    else {
        // Search all source parcels For standalone .md docs.
        CFCParcel **parcels = CFCParcel_all_parcels();
        for (size_t i = 0; parcels[i]; i++) {
            CFCParcel *parcel = parcels[i];
            if (CFCParcel_included(parcel)) { continue; }
            klass = CFCParcel_class_by_full_sym(parcel, full_struct_sym);
            if (klass) { break; }
        }
    }

    FREEMEM(full_struct_sym);
    return klass;
}

static CFCClass*
S_find_class_short(CFCUri *self, const char *struct_sym) {
    CFCClass *klass = NULL;

    if (self->base_class) {
        // Only look in parcel of base class.
        CFCParcel *parcel = CFCClass_get_parcel(self->base_class);
        klass = CFCParcel_class_by_short_sym(parcel, struct_sym);
    }
    else {
        // Search all source parcels For standalone .md docs.
        CFCParcel **parcels = CFCParcel_all_parcels();
        for (size_t i = 0; parcels[i]; i++) {
            CFCParcel *parcel = parcels[i];
            if (CFCParcel_included(parcel)) { continue; }
            CFCClass *candidate
                = CFCParcel_class_by_short_sym(parcel, struct_sym);
            if (candidate && candidate != klass) {
                if (klass) {
                    // Found in multiple parcels.
                    CFCUtil_warn("Ambiguous class name '%s' in URI. Do you"
                                 " mean '%s' or '%s'?", struct_sym,
                                 CFCClass_full_struct_sym(klass),
                                 CFCClass_full_struct_sym(candidate));
                    return NULL;
                }
                klass = candidate;
            }
        }
    }

    return klass;
}

static void
S_set_error(CFCUri *self, const char *error) {
    self->type  = CFC_URI_ERROR;
    self->error = CFCUtil_sprintf("%s: %s", error, self->string);

    CFCUtil_warn(self->error);
}

static char*
S_next_component(char **iter) {
    char *component = *iter;

    if (!component) { return NULL; }

    for (char *ptr = component; *ptr != '\0'; ptr++) {
        if (*ptr == '.') {
            *ptr  = '\0';
            *iter = ptr + 1;
            return component;
        }
    }

    *iter = NULL;
    return component;
}

const char*
CFCUri_get_string(CFCUri *self) {
    return self->string;
}

CFCUriType
CFCUri_get_type(CFCUri *self) {
    if (self->type == 0) { S_parse(self); }
    return self->type;
}

CFCClass*
CFCUri_get_class(CFCUri *self) {
    if (self->type == 0) { S_parse(self); }
    if (self->klass == NULL) {
        CFCUtil_die("Not a class URI");
    }
    return self->klass;
}

CFCDocument*
CFCUri_get_document(CFCUri *self) {
    if (self->type == 0) { S_parse(self); }
    if (self->document == NULL) {
        CFCUtil_die("Not a document URI");
    }
    return self->document;
}

const char*
CFCUri_get_callable_name(CFCUri *self) {
    if (self->type == 0) { S_parse(self); }
    if (self->callable == NULL) {
        CFCUtil_die("Not a callable URI");
    }
    return self->callable;
}

const char*
CFCUri_get_error(CFCUri *self) {
    if (self->type == 0) { S_parse(self); }
    if (self->error == NULL) {
        CFCUtil_die("Not an error URI");
    }
    return self->error;
}


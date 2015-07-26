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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCUri.h"
#include "CFCClass.h"
#include "CFCDocument.h"
#include "CFCUtil.h"

struct CFCUri {
    CFCBase      base;
    char        *string;
    CFCClass    *doc_class;
    int          type;
    CFCClass    *klass;
    CFCDocument *document;
    char        *callable;
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

static char*
S_next_component(char **iter);

int
CFCUri_is_clownfish_uri(const char *uri) {
    return strncmp(uri, "cfish:", 6) == 0;
}

CFCUri*
CFCUri_new(const char *uri, CFCClass *doc_class) {
    CFCUri *self = (CFCUri*)CFCBase_allocate(&CFCURI_META);
    return CFCUri_init(self, uri, doc_class);
}

CFCUri*
CFCUri_init(CFCUri *self, const char *uri, CFCClass *doc_class) {
    CFCUTIL_NULL_CHECK(uri);

    if (strncmp(uri, "cfish:", 6) != 0) {
        CFCUtil_die("Invalid clownfish URI: %s", uri);
    }

    self->string    = CFCUtil_strdup(uri);
    self->doc_class = (CFCClass*)CFCBase_incref((CFCBase*)doc_class);

    return self;
}

void
CFCUri_destroy(CFCUri *self) {
    FREEMEM(self->string);
    FREEMEM(self->callable);
    CFCBase_decref((CFCBase*)self->doc_class);
    CFCBase_decref((CFCBase*)self->klass);
    CFCBase_decref((CFCBase*)self->document);
    CFCBase_destroy((CFCBase*)self);
}

static void
S_parse(CFCUri *self) {
    const char *uri_part = self->string + sizeof("cfish:") - 1;

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

    if (islower(component[0])) {
        // Parcel
        parcel = component;
        component = S_next_component(&iter);
    }

    if (component) {
        if (isupper(component[0])) {
            // Class
            struct_sym = component;
        }
        else if (component == buf && component[0] == '\0' && iter) {
            // "cfish:.Method" style URL.
            ;
        }
        else {
            CFCUtil_die("Invalid component in Clownfish URI: %s",
                        self->string);
        }

        component = S_next_component(&iter);
    }

    if (component) {
        callable = component;
        component = S_next_component(&iter);
    }

    if (component) {
        CFCUtil_die("Trailing components in Clownfish URI: %s", self->string);
    }

    S_resolve(self, parcel, struct_sym, callable);

    FREEMEM(buf);
}

static void
S_resolve(CFCUri *self, const char *parcel, const char *struct_sym,
          const char *callable) {

    // Try to find a CFCClass.
    if (struct_sym || callable) {
        CFCClass *doc_class = self->doc_class;
        CFCClass *klass     = NULL;

        if (parcel) {
            char *full_struct_sym = CFCUtil_sprintf("%s_%s", parcel, struct_sym);
            klass = CFCClass_fetch_by_struct_sym(full_struct_sym);
            FREEMEM(full_struct_sym);
        }
        else if (struct_sym && doc_class) {
            const char *prefix = CFCClass_get_prefix(doc_class);
            char *full_struct_sym = CFCUtil_sprintf("%s%s", prefix, struct_sym);
            klass = CFCClass_fetch_by_struct_sym(full_struct_sym);
            FREEMEM(full_struct_sym);
        }
        else {
            klass = doc_class;
        }

        if (klass) {
            self->type  = CFC_URI_CLASS;
            self->klass = klass;
            CFCBase_incref((CFCBase*)klass);

            if (callable) {
                if (islower(callable[0])) {
                    if (!CFCClass_function(klass, callable)) {
                        CFCUtil_warn("Unknown function '%s' in Clownfish URI: %s",
                                     callable, self->string);
                    }

                    self->type     = CFC_URI_FUNCTION;
                    self->callable = CFCUtil_strdup(callable);
                }
                else {
                    if (!CFCClass_method(klass, callable)) {
                        CFCUtil_warn("Unknown method '%s' in Clownfish URI: %s",
                                     callable, self->string);
                    }

                    self->type     = CFC_URI_METHOD;
                    self->callable = CFCUtil_strdup(callable);
                }
            }
        }
    }

    // Try to find a CFCDocument.
    if (self->type == 0 && !parcel && struct_sym && !callable) {
        CFCDocument *doc = CFCDocument_fetch(struct_sym);

        if (doc) {
            self->type     = CFC_URI_DOCUMENT;
            self->document = doc;
            CFCBase_incref((CFCBase*)doc);
        }
    }

    if (self->type == 0) {
        CFCUtil_die("Couldn't resolve Clownfish URI: %s", self->string);
    }
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

int
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


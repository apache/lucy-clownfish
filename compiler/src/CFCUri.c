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
#include "CFCUtil.h"

struct CFCUri {
    CFCBase base;
    int   type;
    char *prefix;
    char *struct_sym;
    char *full_struct_sym;
    char *func_sym;
};

static const CFCMeta CFCURI_META = {
    "Clownfish::CFC::Uri",
    sizeof(CFCUri),
    (CFCBase_destroy_t)CFCUri_destroy
};

static void
S_parse_uri(CFCUri *self, const char *str, CFCClass *klass);

static char**
S_split(const char *str, char c, size_t *num_parts_ptr);

int
CFCUri_is_clownfish_uri(const char *uri) {
    return strncmp(uri, "cfish:", 6) == 0;
}

CFCUri*
CFCUri_new(const char *uri, CFCClass *klass) {
    CFCUri *self = (CFCUri*)CFCBase_allocate(&CFCURI_META);
    return CFCUri_init(self, uri, klass);
}

CFCUri*
CFCUri_init(CFCUri *self, const char *uri, CFCClass *klass) {
    CFCUTIL_NULL_CHECK(uri);

    if (strncmp(uri, "cfish:", 6) != 0) {
        CFCUtil_die("Invalid clownfish URI: %s", uri);
    }

    if (strcmp(uri + 6, "@null") == 0) {
        self->type = CFC_URI_NULL;
    }
    else {
        S_parse_uri(self, uri, klass);
    }

    if (self->prefix && self->struct_sym) {
        self->full_struct_sym
            = CFCUtil_sprintf("%s%s", self->prefix, self->struct_sym);

        if (getenv("CFC_CHECK_LINKS")) {
            CFCClass *klass
                = CFCClass_fetch_by_struct_sym(self->full_struct_sym);

            if (!klass) {
                CFCUtil_warn("No class found for URI: %s", uri);
            }
            else if (self->type == CFC_URI_FUNCTION) {
                if (!CFCClass_function(klass, self->func_sym)) {
                    const char *class_name = CFCClass_get_name(klass);
                    CFCUtil_warn("No function found for URI %s in %s", uri,
                                 class_name);
                }
            }
            else if (self->type == CFC_URI_METHOD) {
                if (!CFCClass_method(klass, self->func_sym)) {
                    const char *class_name = CFCClass_get_name(klass);
                    CFCUtil_warn("No method found for URI %s in %s", uri,
                                 class_name);
                }
            }
        }
    }

    return self;
}

void
CFCUri_destroy(CFCUri *self) {
    FREEMEM(self->prefix);
    FREEMEM(self->struct_sym);
    FREEMEM(self->full_struct_sym);
    FREEMEM(self->func_sym);
    CFCBase_destroy((CFCBase*)self);
}

static void
S_parse_uri(CFCUri *self, const char *uri, CFCClass *klass) {
    size_t num_components = 0;
    char **components = S_split(uri + 6, '.', &num_components);
    size_t i = 0;

    self->type = CFC_URI_PARCEL;

    if (islower(components[i][0])) {
        // Parcel
        self->prefix = CFCUtil_sprintf("%s_", components[i]);
        ++i;
    }
    else {
        if (!klass) {
            CFCUtil_die("Class needed to complete URI: %s", uri);
        }
        self->prefix = CFCUtil_strdup(CFCClass_get_prefix(klass));
    }

    if (i < num_components) {
        self->type = CFC_URI_CLASS;

        if (isupper(components[i][0])) {
            // Class
            self->struct_sym = components[i];
            components[i] = NULL;
        }
        else if (i == 0 && components[i][0] == '\0') {
            if (!klass) {
                CFCUtil_die("Class needed to complete URI: %s", uri);
            }
            self->struct_sym = CFCUtil_strdup(CFCClass_get_struct_sym(klass));
        }
        else {
            CFCUtil_die("Invalid clownfish URI: %s", uri);
        }

        ++i;
    }

    if (i < num_components) {
        if (isupper(components[i][0])) {
            self->type = CFC_URI_METHOD;
        }
        else if (islower(components[i][0])) {
            self->type = CFC_URI_FUNCTION;
        }
        else {
            CFCUtil_die("Invalid clownfish URI: %s", uri);
        }

        self->func_sym = components[i];
        components[i] = NULL;
        ++i;
    }

    if (i != num_components) {
        CFCUtil_die("Invalid clownfish URI: %s", uri);
    }

    CFCUtil_free_string_array(components);
}

static char**
S_split(const char *str, char c, size_t *num_parts_ptr) {
    const char *ptr;
    int num_parts = 1;

    for (ptr = str; *ptr != '\0'; ++ptr) {
        if (*ptr == c) { ++num_parts; }
    }

    char **parts = (char**)MALLOCATE((num_parts + 1) * sizeof(char*));
    const char *start = str;
    size_t i = 0;

    for (ptr = str; *ptr != '\0'; ++ptr) {
        if (*ptr == c) {
            parts[i++] = CFCUtil_strndup(start, ptr - start);
            start = ptr + 1;
        }
    }
    parts[i++] = CFCUtil_strndup(start, ptr - start);
    parts[i]   = NULL;

    *num_parts_ptr = num_parts;

    return parts;
}

int
CFCUri_get_type(CFCUri *self) {
    return self->type;
}

const char*
CFCUri_get_prefix(CFCUri *self) {
    return self->prefix;
}

const char*
CFCUri_get_struct_sym(CFCUri *self) {
    return self->struct_sym;
}

const char*
CFCUri_full_struct_sym(CFCUri *self) {
    return self->full_struct_sym;
}

const char*
CFCUri_get_func_sym(CFCUri *self) {
    return self->func_sym;
}


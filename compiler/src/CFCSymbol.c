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
#include <ctype.h>

#ifndef true
  #define true 1
  #define false 0
#endif

#define CFC_NEED_SYMBOL_STRUCT_DEF
#include "CFCSymbol.h"
#include "CFCParcel.h"
#include "CFCUtil.h"

static const CFCMeta CFCSYMBOL_META = {
    "Clownfish::CFC::Model::Symbol",
    sizeof(CFCSymbol),
    (CFCBase_destroy_t)CFCSymbol_destroy
};

CFCSymbol*
CFCSymbol_new(struct CFCParcel *parcel, const char *exposure,
              const char *class_name, const char *class_nickname,
              const char *name) {
    CFCSymbol *self = (CFCSymbol*)CFCBase_allocate(&CFCSYMBOL_META);
    return CFCSymbol_init(self, parcel, exposure, class_name, class_nickname,
                          name);
}

static int
S_validate_exposure(const char *exposure) {
    if (!exposure) { return false; }
    if (strcmp(exposure, "public")
        && strcmp(exposure, "parcel")
        && strcmp(exposure, "private")
        && strcmp(exposure, "local")
       ) {
        return false;
    }
    return true;
}

static int
S_validate_class_name(const char *class_name) {
    // The last component must contain lowercase letters (for now).
    const char *last_colon = strrchr(class_name, ':');
    const char *substring = last_colon ? last_colon + 1 : class_name;
    for (;;substring++) {
        if (*substring == 0)          { return false; }
        else if (*substring == ':')   { return false; }
        else if (islower(*substring)) { break; }
    }

    // Must be UpperCamelCase, separated by "::".
    const char *ptr = class_name;
    if (!isupper(*ptr)) { return false; }
    while (*ptr != 0) {
        if (*ptr == 0) { break; }
        else if (*ptr == ':') {
            ptr++;
            if (*ptr != ':') { return false; }
            ptr++;
            if (!isupper(*ptr)) { return false; }
            ptr++;
        }
        else if (!isalnum(*ptr)) { return false; }
        else { ptr++; }
    }

    return true;
}

int
CFCSymbol_validate_class_name_component(const char *name) {
    if (!name || !strlen(name)) { return false; }
    if (!S_validate_class_name(name)) { return false; }
    if (strchr(name, ':') != NULL) { return false; }
    return true;
}

static int
S_validate_class_nickname(const char *class_nickname) {
    // Allow all caps.
    const char *ptr;
    for (ptr = class_nickname; ; ptr++) {
        if (*ptr == 0) {
            if (strlen(class_nickname)) { return true; }
            else { break; }
        }
        else if (!isupper(*ptr)) { break; }
    }

    // Same as one component of a class name.
    return CFCSymbol_validate_class_name_component(class_nickname);
}

static int
S_validate_identifier(const char *identifier) {
    const char *ptr = identifier;
    if (!isalpha(*ptr) && *ptr != '_') { return false; }
    for (; *ptr != 0; ptr++) {
        if (!isalnum(*ptr) && *ptr != '_') { return false; }
    }
    return true;
}

CFCSymbol*
CFCSymbol_init(CFCSymbol *self, struct CFCParcel *parcel,
               const char *exposure, const char *class_name,
               const char *class_nickname, const char *name) {
    // Validate.
    CFCUTIL_NULL_CHECK(parcel);
    if (!S_validate_exposure(exposure)) {
        CFCBase_decref((CFCBase*)self);
        CFCUtil_die("Invalid exposure: '%s'", exposure ? exposure : "[NULL]");
    }
    if (class_name && !S_validate_class_name(class_name)) {
        CFCBase_decref((CFCBase*)self);
        CFCUtil_die("Invalid class_name: '%s'", class_name);
    }
    if (!name || !S_validate_identifier(name)) {
        CFCBase_decref((CFCBase*)self);
        CFCUtil_die("Invalid name: '%s'",  name ? name : "[NULL]");
    }

    // Derive class_nickname if necessary, then validate.
    const char *real_nickname = NULL;
    if (class_name) {
        if (class_nickname) {
            real_nickname = class_nickname;
        }
        else {
            const char *last_colon = strrchr(class_name, ':');
            real_nickname = last_colon ? last_colon + 1 : class_name;
        }
    }
    else if (class_nickname) {
        // Sanity check class_nickname without class_name.
        CFCBase_decref((CFCBase*)self);
        CFCUtil_die("Can't supply class_nickname without class_name");
    }
    else {
        real_nickname = NULL;
    }
    if (real_nickname && !S_validate_class_nickname(real_nickname)) {
        CFCBase_decref((CFCBase*)self);
        CFCUtil_die("Invalid class_nickname: '%s'", real_nickname);
    }

    // Assign.
    self->parcel         = (CFCParcel*)CFCBase_incref((CFCBase*)parcel);
    self->exposure       = CFCUtil_strdup(exposure);
    self->class_name     = CFCUtil_strdup(class_name);
    self->class_nickname = CFCUtil_strdup(real_nickname);
    self->name           = CFCUtil_strdup(name);

    // Derive short_sym and full_sym.
    char *nickname = self->class_nickname ? self->class_nickname : "";
    self->short_sym = CFCUtil_sprintf("%s_%s", nickname, name);
    self->full_sym
        = CFCUtil_sprintf("%s%s", CFCParcel_get_prefix(self->parcel),
                          self->short_sym);

    return self;
}

void
CFCSymbol_destroy(CFCSymbol *self) {
    CFCBase_decref((CFCBase*)self->parcel);
    FREEMEM(self->exposure);
    FREEMEM(self->class_name);
    FREEMEM(self->class_nickname);
    FREEMEM(self->name);
    FREEMEM(self->short_sym);
    FREEMEM(self->full_sym);
    CFCBase_destroy((CFCBase*)self);
}

int
CFCSymbol_equals(CFCSymbol *self, CFCSymbol *other) {
    if (strcmp(self->name, other->name) != 0) { return false; }
    if (!CFCParcel_equals(self->parcel, other->parcel)) { return false; }
    if (strcmp(self->exposure, other->exposure) != 0) { return false; }
    if (self->class_name) {
        if (!other->class_name) { return false; }
        if (strcmp(self->class_name, other->class_name) != 0) {
            return false;
        }
    }
    else if (other->class_name) {
        return false;
    }
    return true;
}

int
CFCSymbol_public(CFCSymbol *self) {
    return !strcmp(self->exposure, "public");
}

int
CFCSymbol_parcel(CFCSymbol *self) {
    return !strcmp(self->exposure, "parcel");
}

int
CFCSymbol_private(CFCSymbol *self) {
    return !strcmp(self->exposure, "private");
}

int
CFCSymbol_local(CFCSymbol *self) {
    return !strcmp(self->exposure, "local");
}

const char*
CFCSymbol_full_sym(CFCSymbol *self) {
    return self->full_sym;
}

const char*
CFCSymbol_short_sym(CFCSymbol *self) {
    return self->short_sym;
}

struct CFCParcel*
CFCSymbol_get_parcel(CFCSymbol *self) {
    return self->parcel;
}

const char*
CFCSymbol_get_class_name(CFCSymbol *self) {
    return self->class_name;
}

const char*
CFCSymbol_get_class_nickname(CFCSymbol *self) {
    return self->class_nickname;
}

const char*
CFCSymbol_get_exposure(CFCSymbol *self) {
    return self->exposure;
}

const char*
CFCSymbol_get_name(CFCSymbol *self) {
    return self->name;
}

const char*
CFCSymbol_get_prefix(CFCSymbol *self) {
    return CFCParcel_get_prefix(self->parcel);
}

const char*
CFCSymbol_get_Prefix(CFCSymbol *self) {
    return CFCParcel_get_Prefix(self->parcel);
}

const char*
CFCSymbol_get_PREFIX(CFCSymbol *self) {
    return CFCParcel_get_PREFIX(self->parcel);
}


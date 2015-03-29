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
#include <stdlib.h>

#include "CFCGoTypeMap.h"
#include "CFCParcel.h"
#include "CFCType.h"
#include "CFCUtil.h"

/* Integer types with implementation-specific widths are tricky to convert.
 * If a C `int` and a Go `int` are not the same width, it is potentially
 * dangerous to map between them.  For example, if a function takes a 32-bit C
 * `int` and we wrap that in a Go function which takes a 64-bit Go `int`, a
 * user who sees the Go function signature will being misled about the range
 * of valid input.
 *
 * Therefore, we take a mostly conservative approach and err on the side of
 * artificially limiting the range.
 */
static struct {
    const char *c;
    const char *go;
} conversions[] = {
    {"bool",     "bool"},
    {"char",     "int8"},
    {"short",    "int16"},
    {"int",      "int32"},
    {"long",     "int32"},
    {"size_t",   "uintptr"},
    {"int8_t",   "int8"},
    {"int16_t",  "int16"},
    {"int32_t",  "int32"},
    {"int64_t",  "int64"},
    {"uint8_t",  "uint8"},
    {"uint16_t", "uint16"},
    {"uint32_t", "uint32"},
    {"uint64_t", "uint64"},
    {"float",    "float32"},
    {"double",   "float64"},
};

static int num_conversions = sizeof(conversions) / sizeof(conversions[0]);

/* TODO: Optimize local conversions by creating a static wrapper function
 * which takes a buffer and allocates memory only if the buffer isn't big
 * enough. */

char*
CFCGoTypeMap_go_type_name(CFCType *type, CFCParcel *current_parcel) {
    if (CFCType_is_object(type)) {
        // Divide the specifier into prefix and struct name.
        const char *specifier  = CFCType_get_specifier(type);
        size_t      prefix_len = 0;
        for (size_t max = strlen(specifier); prefix_len < max; prefix_len++) {
            if (isupper(specifier[prefix_len])) {
                break;
            }
        }
        if (!prefix_len) {
            CFCUtil_die("Can't convert object type name '%s'", specifier);
        }
        const char *struct_sym = specifier + prefix_len;

        // Find the parcel that the type lives in.
        CFCParcel** all_parcels = CFCParcel_all_parcels();
        CFCParcel *parcel = NULL;
        for (int i = 0; all_parcels[i] != NULL; i++) {
            const char *candidate = CFCParcel_get_prefix(all_parcels[i]);
            if (strncmp(candidate, specifier, prefix_len) == 0
                && strlen(candidate) == prefix_len
               ) {
                parcel = all_parcels[i];
                break;
            }
        }
        if (!parcel) {
            CFCUtil_die("Can't find parcel for type '%s'", specifier);
        }

        // If the type lives in this parcel, return only the struct sym
        // without a go package prefix.
        if (parcel == current_parcel) {
            return CFCUtil_strdup(struct_sym);
        }

        // The type lives in another parcel, so prefix its Go package name.
        // TODO: Stop downcasing once Clownfish parcel names are constrained
        // to lower case.
        const char *package_name = CFCParcel_get_name(parcel);
        if (strrchr(package_name, '.')) {
            package_name = strrchr(package_name, '.') + 1;
        }
        char *result = CFCUtil_sprintf("%s.%s", package_name, struct_sym);
        for (int i = 0; result[i] != '.'; i++) {
            result[i] = tolower(result[i]);
        }
        return result;
    }
    else if (CFCType_is_primitive(type)) {
        const char *specifier = CFCType_get_specifier(type);
        for (int i = 0; i < num_conversions; i++) {
            if (strcmp(specifier, conversions[i].c) == 0) {
                return CFCUtil_strdup(conversions[i].go);
            }
        }
    }

    return NULL;
}

char*
CFCGoTypeMap_go_short_package(CFCParcel *parcel) {
    // The Go short package name is the last component of the dot-separated
    // Clownfish parcel name.
    const char *parcel_frag = strrchr(CFCParcel_get_name(parcel), '.');
    if (parcel_frag) {
        parcel_frag += 1;
    }
    else {
        parcel_frag = CFCParcel_get_name(parcel);
    }
    // TODO: Don't downcase package name once caps are forbidden in Clownfish
    // parcel names.
    char *go_short_package = CFCUtil_strdup(parcel_frag);
    for (int i = 0; go_short_package[i] != '\0'; i++) {
        go_short_package[i] = tolower(go_short_package[i]);
    }
    return go_short_package;
}


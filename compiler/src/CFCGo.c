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

#include "charmony.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCGo.h"
#include "CFCParcel.h"
#include "CFCClass.h"
#include "CFCMethod.h"
#include "CFCHierarchy.h"
#include "CFCUtil.h"
#include "CFCGoTypeMap.h"

static void
S_CFCGo_destroy(CFCGo *self);

struct CFCGo {
    CFCBase base;
    CFCHierarchy *hierarchy;
    char *header;
    char *footer;
    char *c_header;
    char *c_footer;
};

static const CFCMeta CFCGO_META = {
    "Clownfish::CFC::Binding::Go",
    sizeof(CFCGo),
    (CFCBase_destroy_t)S_CFCGo_destroy
};

CFCGo*
CFCGo_new(CFCHierarchy *hierarchy) {
    CFCUTIL_NULL_CHECK(hierarchy);
    CFCGo *self = (CFCGo*)CFCBase_allocate(&CFCGO_META);
    self->hierarchy  = (CFCHierarchy*)CFCBase_incref((CFCBase*)hierarchy);
    self->header     = CFCUtil_strdup("");
    self->footer     = CFCUtil_strdup("");
    self->c_header   = CFCUtil_strdup("");
    self->c_footer   = CFCUtil_strdup("");
    return self;
}

static void
S_CFCGo_destroy(CFCGo *self) {
    CFCBase_decref((CFCBase*)self->hierarchy);
    FREEMEM(self->header);
    FREEMEM(self->footer);
    FREEMEM(self->c_header);
    FREEMEM(self->c_footer);
    CFCBase_destroy((CFCBase*)self);
}

void
CFCGo_set_header(CFCGo *self, const char *header) {
    CFCUTIL_NULL_CHECK(header);
    free(self->header);
    self->header = CFCUtil_strdup(header);
    free(self->c_header);
    self->c_header = CFCUtil_make_c_comment(header);
}

void
CFCGo_set_footer(CFCGo *self, const char *footer) {
    CFCUTIL_NULL_CHECK(footer);
    free(self->footer);
    self->footer = CFCUtil_strdup(footer);
    free(self->c_footer);
    self->c_footer = CFCUtil_make_c_comment(footer);
}

static void
S_write_hostdefs(CFCGo *self) {
    const char pattern[] =
        "/*\n"
        " * %s\n"
        " */\n"
        "\n"
        "#ifndef H_CFISH_HOSTDEFS\n"
        "#define H_CFISH_HOSTDEFS 1\n"
        "\n"
        "#define CFISH_NO_DYNAMIC_OVERRIDES\n"
        "\n"
        "#define CFISH_OBJ_HEAD \\\n"
        "    size_t refcount;\n"
        "\n"
        "#endif /* H_CFISH_HOSTDEFS */\n"
        "\n"
        "%s\n";
    char *content
        = CFCUtil_sprintf(pattern, self->header, self->footer);

    // Write if the content has changed.
    const char *inc_dest = CFCHierarchy_get_include_dest(self->hierarchy);
    char *filepath = CFCUtil_sprintf("%s" CHY_DIR_SEP "cfish_hostdefs.h",
                                     inc_dest);
    CFCUtil_write_if_changed(filepath, content, strlen(content));

    FREEMEM(filepath);
    FREEMEM(content);
}

void
CFCGo_write_bindings(CFCGo *self, CFCParcel *parcel, const char *dest) {
    CHY_UNUSED_VAR(parcel);
    CHY_UNUSED_VAR(dest);
    S_write_hostdefs(self);
}


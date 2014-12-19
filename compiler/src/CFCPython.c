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
#include "CFCPython.h"
#include "CFCParcel.h"
#include "CFCClass.h"
#include "CFCMethod.h"
#include "CFCHierarchy.h"
#include "CFCUtil.h"
#include "CFCBindCore.h"

struct CFCPython {
    CFCBase base;
    CFCHierarchy *hierarchy;
    char *header;
    char *footer;
};

void
S_destroy(CFCPython *self);

static const CFCMeta CFCPYTHON_META = {
    "Clownfish::CFC::Binding::Python",
    sizeof(CFCPython),
    (CFCBase_destroy_t)S_destroy
};

CFCPython*
CFCPython_new(CFCHierarchy *hierarchy) {
    CFCUTIL_NULL_CHECK(hierarchy);
    CFCPython *self = (CFCPython*)CFCBase_allocate(&CFCPYTHON_META);
    self->hierarchy  = (CFCHierarchy*)CFCBase_incref((CFCBase*)hierarchy);
    self->header     = CFCUtil_strdup("");
    self->footer     = CFCUtil_strdup("");
    return self;
}

void
S_destroy(CFCPython *self) {
    CFCBase_decref((CFCBase*)self->hierarchy);
    FREEMEM(self->header);
    FREEMEM(self->footer);
    CFCBase_destroy((CFCBase*)self);
}

void
CFCPython_set_header(CFCPython *self, const char *header) {
    CFCUTIL_NULL_CHECK(header);
    free(self->header);
    self->header = CFCUtil_make_c_comment(header);
}

void
CFCPython_set_footer(CFCPython *self, const char *footer) {
    CFCUTIL_NULL_CHECK(footer);
    free(self->footer);
    self->footer = CFCUtil_make_c_comment(footer);
}

static void
S_write_hostdefs(CFCPython *self) {
    const char pattern[] =
        "%s\n"
        "\n"
        "#ifndef H_CFISH_HOSTDEFS\n"
        "#define H_CFISH_HOSTDEFS 1\n"
        "\n"
        "#include \"Python.h\"\n"
        "\n"
        "#define CFISH_OBJ_HEAD \\\n"
        "    PyObject_HEAD\n"
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
CFCPython_write_bindings(CFCPython *self, const char *parcel_name, const char *dest) {
    S_write_hostdefs(self);
}


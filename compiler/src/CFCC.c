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

#include <stdio.h>
#include <string.h>

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCC.h"
#include "CFCCMan.h"
#include "CFCClass.h"
#include "CFCHierarchy.h"
#include "CFCMethod.h"
#include "CFCUtil.h"

struct CFCC {
    CFCBase base;
    CFCHierarchy *hierarchy;
    char         *c_header;
    char         *c_footer;
    char         *man_header;
    char         *man_footer;
};

static const CFCMeta CFCC_META = {
    "Clownfish::CFC::Binding::C",
    sizeof(CFCC),
    (CFCBase_destroy_t)CFCC_destroy
};

static char*
S_callback_decs(CFCClass *klass);

CFCC*
CFCC_new(CFCHierarchy *hierarchy, const char *header, const char *footer) {
    CFCC *self = (CFCC*)CFCBase_allocate(&CFCC_META);
    return CFCC_init(self, hierarchy, header, footer);
}

CFCC*
CFCC_init(CFCC *self, CFCHierarchy *hierarchy, const char *header,
          const char *footer) {
    CFCUTIL_NULL_CHECK(hierarchy);
    CFCUTIL_NULL_CHECK(header);
    CFCUTIL_NULL_CHECK(footer);
    self->hierarchy  = (CFCHierarchy*)CFCBase_incref((CFCBase*)hierarchy);
    self->c_header   = CFCUtil_make_c_comment(header);
    self->c_footer   = CFCUtil_make_c_comment(footer);
    self->man_header = CFCUtil_make_troff_comment(header);
    self->man_footer = CFCUtil_make_troff_comment(footer);
    return self;
}

void
CFCC_destroy(CFCC *self) {
    CFCBase_decref((CFCBase*)self->hierarchy);
    FREEMEM(self->c_header);
    FREEMEM(self->c_footer);
    FREEMEM(self->man_header);
    FREEMEM(self->man_footer);
    CFCBase_destroy((CFCBase*)self);
}

/* Write "callbacks.h" with NULL callbacks.
 */
void
CFCC_write_callbacks(CFCC *self) {
    CFCHierarchy  *hierarchy   = self->hierarchy;
    CFCClass     **ordered     = CFCHierarchy_ordered_classes(hierarchy);
    char          *all_cb_decs = CFCUtil_strdup("");

    for (int i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];

        if (!CFCClass_included(klass)) {
            char *cb_decs = S_callback_decs(klass);
            all_cb_decs = CFCUtil_cat(all_cb_decs, cb_decs, NULL);
            FREEMEM(cb_decs);
        }
    }

    FREEMEM(ordered);

    const char pattern[] =
        "%s\n"
        "#ifndef CFCCALLBACKS_H\n"
        "#define CFCCALLBACKS_H 1\n"
        "\n"
        "#include <stddef.h>\n"
        "\n"
        "%s"
        "\n"
        "#endif /* CFCCALLBACKS_H */\n"
        "\n"
        "%s\n"
        "\n";
    char *file_content = CFCUtil_sprintf(pattern, self->c_header, all_cb_decs,
                                         self->c_footer);

    // Unlink then write file.
    const char *inc_dest = CFCHierarchy_get_include_dest(hierarchy);
    char *filepath = CFCUtil_sprintf("%s" CHY_DIR_SEP "callbacks.h", inc_dest);
    remove(filepath);
    CFCUtil_write_file(filepath, file_content, strlen(file_content));
    FREEMEM(filepath);

    FREEMEM(all_cb_decs);
    FREEMEM(file_content);
}

static char*
S_callback_decs(CFCClass *klass) {
    CFCMethod **fresh_methods = CFCClass_fresh_methods(klass);
    char       *cb_decs       = CFCUtil_strdup("");

    for (int meth_num = 0; fresh_methods[meth_num] != NULL; meth_num++) {
        CFCMethod *method = fresh_methods[meth_num];

        // Define callback to NULL.
        if (CFCMethod_novel(method) && !CFCMethod_final(method)) {
            const char *override_sym = CFCMethod_full_override_sym(method);
            cb_decs = CFCUtil_cat(cb_decs, "#define ", override_sym, " NULL\n",
                                  NULL);
        }
    }

    FREEMEM(fresh_methods);

    return cb_decs;
}

void
CFCC_write_man_pages(CFCC *self) {
    CFCHierarchy  *hierarchy = self->hierarchy;
    CFCClass     **ordered   = CFCHierarchy_ordered_classes(hierarchy);

    size_t num_classes = 0;
    for (size_t i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        if (!CFCClass_included(klass)) { ++num_classes; }
    }
    char **man_pages = (char**)CALLOCATE(num_classes, sizeof(char*));

    // Generate man pages, but don't write.  That way, if there's an error
    // while generating the pages, we leak memory but don't clutter up the file 
    // system.
    for (size_t i = 0, j = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        if (CFCClass_included(klass)) { continue; }

        char *man_page = CFCCMan_create_man_page(klass);
        man_pages[j++] = man_page;
    }

    const char *dest = CFCHierarchy_get_dest(hierarchy);
    char *man3_path
        = CFCUtil_sprintf("%s" CHY_DIR_SEP "man" CHY_DIR_SEP "man3", dest);
    if (!CFCUtil_is_dir(man3_path)) {
        CFCUtil_make_path(man3_path);
        if (!CFCUtil_is_dir(man3_path)) {
            CFCUtil_die("Can't make path %s", man3_path);
        }
    }

    // Write out any man pages that have changed.
    for (size_t i = 0, j = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        if (CFCClass_included(klass)) { continue; }

        char *raw_man_page = man_pages[j++];
        if (!raw_man_page) { continue; }
        char *man_page = CFCUtil_sprintf("%s%s%s", self->man_header,
                                         raw_man_page, self->man_footer);

        const char *full_struct_sym = CFCClass_full_struct_sym(klass);
        char *filename = CFCUtil_sprintf("%s" CHY_DIR_SEP "%s.3", man3_path,
                                         full_struct_sym);
        CFCUtil_write_if_changed(filename, man_page, strlen(man_page));
        FREEMEM(filename);
        FREEMEM(man_page);
        FREEMEM(raw_man_page);
    }

    FREEMEM(man3_path);
    FREEMEM(man_pages);
    FREEMEM(ordered);
}

void
CFCC_write_hostdefs(CFCC *self) {
    const char pattern[] =
        "%s\n"
        "\n"
        "#ifndef H_CFISH_HOSTDEFS\n"
        "#define H_CFISH_HOSTDEFS 1\n"
        "\n"
        "#define CFISH_OBJ_HEAD \\\n"
        "    size_t refcount;\n"
        "\n"
        "#endif /* H_CFISH_HOSTDEFS */\n"
        "\n"
        "%s\n";
    char *content
        = CFCUtil_sprintf(pattern, self->c_header, self->c_footer);

    // Unlink then write file.
    const char *inc_dest = CFCHierarchy_get_include_dest(self->hierarchy);
    char *filepath = CFCUtil_sprintf("%s" CHY_DIR_SEP "cfish_hostdefs.h",
                                     inc_dest);
    remove(filepath);
    CFCUtil_write_file(filepath, content, strlen(content));
    FREEMEM(filepath);

    FREEMEM(content);
}



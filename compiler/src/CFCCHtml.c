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

#include <cmark.h>

#include "charmony.h"

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCCHtml.h"
#include "CFCC.h"
#include "CFCClass.h"
#include "CFCDocuComment.h"
#include "CFCFunction.h"
#include "CFCHierarchy.h"
#include "CFCMethod.h"
#include "CFCParamList.h"
#include "CFCParcel.h"
#include "CFCSymbol.h"
#include "CFCType.h"
#include "CFCUtil.h"
#include "CFCUri.h"
#include "CFCVariable.h"

#ifndef true
    #define true 1
    #define false 0
#endif

#define UTF8_NDASH "\xE2\x80\x93"

struct CFCCHtml {
    CFCBase base;
    CFCHierarchy *hierarchy;
    char *doc_path;
    char *header;
    char *footer;
};

static const CFCMeta CFCCHTML_META = {
    "Clownfish::CFC::Binding::C::Html",
    sizeof(CFCCHtml),
    (CFCBase_destroy_t)CFCCHtml_destroy
};

static const char header_template[] =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "<meta charset=\"utf-8\">\n"
    "{autogen_header}"
    "<meta name=\"viewport\" content=\"width=device-width\" />\n"
    "<title>{title}</title>\n"
    "<style type=\"text/css\">\n"
    "body {\n"
    "    max-width: 48em;\n"
    "    font: 0.85em/1.4 sans-serif;\n"
    "}\n"
    "a {\n"
    "    color: #23b;\n"
    "}\n"
    "table {\n"
    "    border-collapse: collapse;\n"
    "}\n"
    "td {\n"
    "    padding: 0;\n"
    "}\n"
    "td.label {\n"
    "    padding-right: 2em;\n"
    "    font-weight: bold;\n"
    "}\n"
    "dt {\n"
    "    font-weight: bold;\n"
    "}\n"
    "pre {\n"
    "    border: 1px solid #ccc;\n"
    "    padding: 0.2em 0.4em;\n"
    "    background: #f6f6f6;\n"
    "    font-size: 0.92em;\n"
    "}\n"
    "pre a {\n"
    "    text-decoration: none;\n"
    "}\n"
    "pre, code {\n"
    "    font-family: \"Consolas\", \"Menlo\", monospace;\n"
    "}\n"
    "span.prefix, span.comment {\n"
    "    color: #888;\n"
    "}\n"
    "</style>\n"
    "</head>\n"
    "<body>\n";

static const char footer_template[] =
    "</body>\n"
    "</html>\n"
    "{autogen_footer}";

static int
S_compare_class_name(const void *va, const void *vb);

static char*
S_index_filename(CFCParcel *parcel);

static char*
S_html_create_name(CFCClass *klass);

static char*
S_html_create_synopsis(CFCClass *klass);

static char*
S_html_create_description(CFCClass *klass);

static char*
S_html_create_functions(CFCClass *klass);

static char*
S_html_create_methods(CFCClass *klass);

static char*
S_html_create_fresh_methods(CFCClass *klass, CFCClass *ancestor);

static char*
S_html_create_func(CFCClass *klass, CFCFunction *func, const char *prefix,
                   const char *short_sym);

static char*
S_html_create_param_list(CFCClass *klass, CFCFunction *func);

static char*
S_html_create_inheritance(CFCClass *klass);

static char*
S_md_to_html(CFCClass *klass, const char *md);

static void
S_convert_uris(CFCClass *klass, cmark_node *node);

static void
S_convert_uri(CFCClass *klass, cmark_node *link);

static char*
S_type_to_html(CFCClass *klass, CFCType *type);

CFCCHtml*
CFCCHtml_new(CFCHierarchy *hierarchy, const char *header, const char *footer) {
    CFCCHtml *self = (CFCCHtml*)CFCBase_allocate(&CFCCHTML_META);
    return CFCCHtml_init(self, hierarchy, header, footer);
}

CFCCHtml*
CFCCHtml_init(CFCCHtml *self, CFCHierarchy *hierarchy, const char *header,
              const char *footer) {
    CFCUTIL_NULL_CHECK(hierarchy);
    CFCUTIL_NULL_CHECK(header);
    CFCUTIL_NULL_CHECK(footer);

    self->hierarchy = (CFCHierarchy*)CFCBase_incref((CFCBase*)hierarchy);

    const char *dest = CFCHierarchy_get_dest(hierarchy);
    self->doc_path
        = CFCUtil_sprintf("%s" CHY_DIR_SEP "share" CHY_DIR_SEP "doc"
                          CHY_DIR_SEP "clownfish", dest);

    char *header_comment = CFCUtil_make_html_comment(header);
    char *footer_comment = CFCUtil_make_html_comment(footer);
    self->header = CFCUtil_global_replace(header_template, "{autogen_header}",
                                          header_comment);
    self->footer = CFCUtil_global_replace(footer_template, "{autogen_footer}",
                                          footer_comment);
    FREEMEM(footer_comment);
    FREEMEM(header_comment);

    return self;
}

void
CFCCHtml_destroy(CFCCHtml *self) {
    CFCBase_decref((CFCBase*)self->hierarchy);
    FREEMEM(self->doc_path);
    FREEMEM(self->header);
    FREEMEM(self->footer);
    CFCBase_destroy((CFCBase*)self);
}

void
CFCCHtml_write_html_docs(CFCCHtml *self) {
    CFCHierarchy  *hierarchy = self->hierarchy;
    CFCClass     **ordered   = CFCHierarchy_ordered_classes(hierarchy);
    CFCParcel    **parcels   = CFCParcel_all_parcels();
    const char    *doc_path  = self->doc_path;

    size_t num_parcels = 0;
    for (size_t i = 0; parcels[i] != NULL; i++) {
        ++num_parcels;
    }

    size_t num_classes = 0;
    for (size_t i = 0; ordered[i] != NULL; i++) {
        ++num_classes;
    }

    qsort(ordered, num_classes, sizeof(*ordered), S_compare_class_name);

    size_t   max_docs  = num_classes + num_parcels;
    char   **filenames = (char**)CALLOCATE(max_docs, sizeof(char*));
    char   **html_docs = (char**)CALLOCATE(max_docs, sizeof(char*));
    size_t   num_docs  = 0;

    // Generate HTML docs, but don't write.  That way, if there's an error
    // while generating the pages, we leak memory but don't clutter up the file
    // system.

    for (size_t i = 0; parcels[i] != NULL; ++i) {
        CFCParcel *parcel = parcels[i];
        if (CFCParcel_included(parcel)) { continue; }

        char *html = CFCCHtml_create_index_doc(self, parcel, ordered);
        if (html != NULL) {
            filenames[num_docs] = S_index_filename(parcel);
            html_docs[num_docs] = html;
            ++num_docs;
        }
    }

    for (size_t i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        if (CFCClass_included(klass) || !CFCClass_public(klass)) {
            continue;
        }

        const char *full_struct_sym = CFCClass_full_struct_sym(klass);
        filenames[num_docs] = CFCUtil_sprintf("%s.html", full_struct_sym);
        html_docs[num_docs] = CFCCHtml_create_html_doc(self, klass);
        ++num_docs;
    }

    if (!CFCUtil_is_dir(doc_path)) {
        CFCUtil_make_path(doc_path);
        if (!CFCUtil_is_dir(doc_path)) {
            CFCUtil_die("Can't make path %s", doc_path);
        }
    }

    for (size_t i = 0; i < num_docs; ++i) {
        char *filename = filenames[i];
        char *path     = CFCUtil_sprintf("%s" CHY_DIR_SEP "%s", doc_path,
                                         filename);
        char *html_doc = html_docs[i];
        CFCUtil_write_if_changed(path, html_doc, strlen(html_doc));
        FREEMEM(html_doc);
        FREEMEM(path);
        FREEMEM(filename);
    }

    FREEMEM(html_docs);
    FREEMEM(filenames);
    FREEMEM(ordered);
}

char*
CFCCHtml_create_index_doc(CFCCHtml *self, CFCParcel *parcel,
                          CFCClass **classes) {
    const char *prefix      = CFCParcel_get_prefix(parcel);
    const char *parcel_name = CFCParcel_get_name(parcel);
    char *class_list = CFCUtil_strdup("");

    for (size_t i = 0; classes[i] != NULL; i++) {
        CFCClass *klass = classes[i];
        if (strcmp(CFCClass_get_prefix(klass), prefix) != 0
            || !CFCClass_public(klass)
        ) {
            continue;
        }

        const char *struct_sym = CFCClass_full_struct_sym(klass);
        const char *class_name = CFCClass_get_name(klass);
        class_list
            = CFCUtil_cat(class_list, "<li><a href=\"", struct_sym, ".html\">",
                          class_name, "</a></li>\n", NULL);
    }

    if (class_list[0] == '\0') {
        FREEMEM(class_list);
        return NULL;
    }

    char *title
        = CFCUtil_sprintf("%s " UTF8_NDASH " C API Index", parcel_name);
    char *header = CFCUtil_global_replace(self->header, "{title}", title);

    const char pattern[] =
        "%s"
        "<h1>%s</h1>\n"
        "<ul>\n"
        "%s"
        "</ul>\n"
        "%s";
    char *html_doc
        = CFCUtil_sprintf(pattern, header, title, class_list, self->footer);

    FREEMEM(header);
    FREEMEM(title);
    FREEMEM(class_list);

    return html_doc;
}

char*
CFCCHtml_create_html_doc(CFCCHtml *self, CFCClass *klass) {
    const char *class_name     = CFCClass_get_name(klass);
    char *title
        = CFCUtil_sprintf("%s " UTF8_NDASH " C API Documentation", class_name);
    char *header = CFCUtil_global_replace(self->header, "{title}", title);
    char *body = CFCCHtml_create_html_body(klass);

    char *html_doc = CFCUtil_sprintf("%s%s%s", header, body, self->footer);

    FREEMEM(body);
    FREEMEM(header);
    FREEMEM(title);
    return html_doc;
}

char*
CFCCHtml_create_html_body(CFCClass *klass) {
    CFCParcel  *parcel         = CFCClass_get_parcel(klass);
    const char *parcel_name    = CFCParcel_get_name(parcel);
    const char *class_name     = CFCClass_get_name(klass);
    const char *class_nickname = CFCClass_get_nickname(klass);
    const char *class_var      = CFCClass_full_class_var(klass);
    const char *struct_sym     = CFCClass_full_struct_sym(klass);

    // Create NAME.
    char *name = S_html_create_name(klass);

    // Create SYNOPSIS.
    char *synopsis = S_html_create_synopsis(klass);

    // Create DESCRIPTION.
    char *description = S_html_create_description(klass);

    // Create CONSTRUCTORS.
    char *functions_html = S_html_create_functions(klass);

    // Create METHODS, possibly including an ABSTRACT METHODS section.
    char *methods_html = S_html_create_methods(klass);

    // Build an INHERITANCE section describing class ancestry.
    char *inheritance = S_html_create_inheritance(klass);

    char *index_filename = S_index_filename(parcel);

    // Put it all together.
    const char pattern[] =
        "<h1>%s</h1>\n"
        "<table>\n"
        "<tr>\n"
        "<td class=\"label\">parcel</td>\n"
        "<td><a href=\"%s\">%s</a></td>\n"
        "</tr>\n"
        "<tr>\n"
        "<td class=\"label\">class name</td>\n"
        "<td>%s</td>\n"
        "</tr>\n"
        "<tr>\n"
        "<td class=\"label\">class nickname</td>\n"
        "<td>%s</td>\n"
        "</tr>\n"
        "<tr>\n"
        "<td class=\"label\">class variable</td>\n"
        "<td><code>%s</code></td>\n"
        "</tr>\n"
        "<tr>\n"
        "<td class=\"label\">struct symbol</td>\n"
        "<td><code>%s</code></td>\n"
        "</tr>\n"
        "</table>\n"
        "%s"
        "%s"
        "%s"
        "%s"
        "%s"
        "%s";
    char *html_body
        = CFCUtil_sprintf(pattern, class_name, index_filename,
                          parcel_name, class_name, class_nickname, class_var,
                          struct_sym, name, synopsis, description,
                          functions_html, methods_html, inheritance);

    FREEMEM(index_filename);
    FREEMEM(name);
    FREEMEM(synopsis);
    FREEMEM(description);
    FREEMEM(functions_html);
    FREEMEM(methods_html);
    FREEMEM(inheritance);

    return html_body;
}

static int
S_compare_class_name(const void *va, const void *vb) {
    const char *a = CFCClass_get_name(*(CFCClass**)va);
    const char *b = CFCClass_get_name(*(CFCClass**)vb);

    return strcmp(a, b);
}

static char*
S_index_filename(CFCParcel *parcel) {
    char *nickname = CFCUtil_strdup(CFCParcel_get_nickname(parcel));
    for (size_t i = 0; nickname[i]; ++i) {
        nickname[i] = tolower(nickname[i]);
    }
    char *filename = CFCUtil_sprintf("%s.html", nickname);
    FREEMEM(nickname);
    return filename;
}

static char*
S_html_create_name(CFCClass *klass) {
    const char     *class_name = CFCClass_get_name(klass);
    char           *md         = CFCUtil_strdup(class_name);
    CFCDocuComment *docucom    = CFCClass_get_docucomment(klass);

    if (docucom) {
        const char *raw_brief = CFCDocuComment_get_brief(docucom);
        if (raw_brief && raw_brief[0] != '\0') {
            md = CFCUtil_cat(md, " " UTF8_NDASH " ", raw_brief, NULL);
        }
    }

    char *html = S_md_to_html(klass, md);

    const char *format =
        "<h2>Name</h2>\n"
        "%s";
    char *result = CFCUtil_sprintf(format, html);

    FREEMEM(html);
    FREEMEM(md);
    return result;
}

static char*
S_html_create_synopsis(CFCClass *klass) {
    CHY_UNUSED_VAR(klass);
    return CFCUtil_strdup("");
}

static char*
S_html_create_description(CFCClass *klass) {
    CFCDocuComment *docucom = CFCClass_get_docucomment(klass);
    char           *desc    = NULL;

    if (docucom) {
        const char *raw_desc = CFCDocuComment_get_long(docucom);
        if (raw_desc && raw_desc[0] != '\0') {
            desc = S_md_to_html(klass, raw_desc);
        }
    }

    if (!desc) { return CFCUtil_strdup(""); }

    char *result = CFCUtil_sprintf("<h2>Description</h2>\n%s", desc);

    FREEMEM(desc);
    return result;
}

static char*
S_html_create_functions(CFCClass *klass) {
    CFCFunction **functions = CFCClass_functions(klass);
    const char   *prefix    = CFCClass_get_prefix(klass);
    char         *result    = CFCUtil_strdup("");

    for (int func_num = 0; functions[func_num] != NULL; func_num++) {
        CFCFunction *func = functions[func_num];
        if (!CFCFunction_public(func)) { continue; }

        if (result[0] == '\0') {
            result = CFCUtil_cat(result, "<h2>Functions</h2>\n<dl>\n", NULL);
        }

        const char *name = CFCFunction_get_name(func);
        result = CFCUtil_cat(result, "<dt id=\"func_", name, "\">",
                             name, "</dt>\n", NULL);

        char *short_sym = CFCFunction_short_func_sym(func, klass);
        char *func_html = S_html_create_func(klass, func, prefix, short_sym);
        result = CFCUtil_cat(result, func_html, NULL);
        FREEMEM(func_html);
        FREEMEM(short_sym);
    }

    if (result[0] != '\0') {
        result = CFCUtil_cat(result, "</dl>\n", NULL);
    }

    return result;
}

static char*
S_html_create_methods(CFCClass *klass) {
    char *methods_html  = CFCUtil_strdup("");
    char *result;

    for (CFCClass *ancestor = klass;
         ancestor;
         ancestor = CFCClass_get_parent(ancestor)
    ) {
        const char *class_name = CFCClass_get_name(ancestor);
        // Exclude methods inherited from Clownfish::Obj
        if (ancestor != klass && strcmp(class_name, "Clownfish::Obj") == 0) {
            break;
        }

        char *fresh_html = S_html_create_fresh_methods(klass, ancestor);
        if (fresh_html[0] != '\0') {
            if (ancestor == klass) {
                methods_html = CFCUtil_cat(methods_html, fresh_html, NULL);
            }
            else {
                methods_html
                    = CFCUtil_cat(methods_html, "<h3>Methods inherited from ",
                                  class_name, "</h3>\n", fresh_html, NULL);
            }
        }
        FREEMEM(fresh_html);
    }

    if (methods_html[0] == '\0') {
        result = CFCUtil_strdup("");
    }
    else {
        result = CFCUtil_sprintf("<h2>Methods</h2>\n%s", methods_html);
    }

    FREEMEM(methods_html);
    return result;
}

/** Return HTML for the fresh methods of `ancestor`.
 */
static char*
S_html_create_fresh_methods(CFCClass *klass, CFCClass *ancestor) {
    CFCMethod  **fresh_methods = CFCClass_fresh_methods(klass);
    const char  *prefix        = CFCClass_get_prefix(klass);
    char        *result        = CFCUtil_strdup("");

    for (int meth_num = 0; fresh_methods[meth_num] != NULL; meth_num++) {
        CFCMethod *method = fresh_methods[meth_num];
        if (!CFCMethod_public(method)) {
            continue;
        }

        if (!CFCMethod_is_fresh(method, ancestor)) {
            // The method is implementated in a subclass and already
            // documented.
            continue;
        }

        if (result[0] == '\0') {
            result = CFCUtil_cat(result, "<dl>\n", NULL);
        }

        const char *name = CFCMethod_get_name(method);
        result = CFCUtil_cat(result, "<dt id=\"func_", name, "\">",
                             name, NULL);
        if (CFCMethod_abstract(method)) {
            result = CFCUtil_cat(result,
                    " <span class=\"comment\">(abstract)</span>", NULL);
        }
        result = CFCUtil_cat(result, "</dt>\n", NULL);

        char       *short_sym = CFCMethod_short_method_sym(method, klass);
        char *method_html = S_html_create_func(klass, (CFCFunction*)method,
                                               prefix, short_sym);
        result = CFCUtil_cat(result, method_html, NULL);
        FREEMEM(method_html);
        FREEMEM(short_sym);
    }

    if (result[0] != '\0') {
        result = CFCUtil_cat(result, "</dl>\n", NULL);
    }

    return result;
}

static char*
S_html_create_func(CFCClass *klass, CFCFunction *func, const char *prefix,
                   const char *short_sym) {
    CFCType    *return_type      = CFCFunction_get_return_type(func);
    char       *return_type_html = S_type_to_html(klass, return_type);
    const char *incremented      = "";

    if (CFCType_incremented(return_type)) {
        incremented = " <span class=\"comment\">// incremented</span>";
    }

    char *param_list = S_html_create_param_list(klass, func);

    const char *pattern =
        "<dd>\n"
        "<pre><code>%s%s\n"
        "<span class=\"prefix\">%s</span><strong>%s</strong>%s</code></pre>\n";
    char *result = CFCUtil_sprintf(pattern, return_type_html, incremented,
                                   prefix, short_sym, param_list);

    FREEMEM(param_list);

    // Get documentation, which may be inherited.
    CFCDocuComment *docucomment = CFCFunction_get_docucomment(func);
    if (!docucomment) {
        const char *name = CFCFunction_get_name(func);
        CFCClass *parent = klass;
        while (NULL != (parent = CFCClass_get_parent(parent))) {
            CFCFunction *parent_func
                = (CFCFunction*)CFCClass_method(parent, name);
            if (!parent_func) { break; }
            docucomment = CFCFunction_get_docucomment(parent_func);
            if (docucomment) { break; }
        }
    }

    if (docucomment) {
        // Description
        const char *raw_desc = CFCDocuComment_get_description(docucomment);
        char *desc = S_md_to_html(klass, raw_desc);
        result = CFCUtil_cat(result, desc, NULL);
        FREEMEM(desc);

        // Params
        const char **param_names
            = CFCDocuComment_get_param_names(docucomment);
        const char **param_docs
            = CFCDocuComment_get_param_docs(docucomment);
        if (param_names[0]) {
            result = CFCUtil_cat(result, "<dl>\n", NULL);
            for (size_t i = 0; param_names[i] != NULL; i++) {
                char *doc = S_md_to_html(klass, param_docs[i]);
                result = CFCUtil_cat(result, "<dt>", param_names[i],
                                     "</dt>\n<dd>", doc, "</dd>\n",
                                     NULL);
                FREEMEM(doc);
            }
            result = CFCUtil_cat(result, "</dl>\n", NULL);
        }

        // Return value
        const char *retval_doc = CFCDocuComment_get_retval(docucomment);
        if (retval_doc && strlen(retval_doc)) {
            char *md = CFCUtil_sprintf("**Returns:** %s", retval_doc);
            char *html = S_md_to_html(klass, md);
            result = CFCUtil_cat(result, html, NULL);
            FREEMEM(html);
            FREEMEM(md);
        }
    }

    result = CFCUtil_cat(result, "</dd>\n", NULL);

    FREEMEM(return_type_html);
    return result;
}

static char*
S_html_create_param_list(CFCClass *klass, CFCFunction *func) {
    CFCParamList  *param_list = CFCFunction_get_param_list(func);
    CFCVariable  **variables  = CFCParamList_get_variables(param_list);

    const char *cfc_class = CFCBase_get_cfc_class((CFCBase*)func);
    int is_method = strcmp(cfc_class, "Clownfish::CFC::Model::Method") == 0;

    if (!variables[0]) {
        return CFCUtil_strdup("(void);\n");
    }

    char *result = CFCUtil_strdup("(");

    for (int i = 0; variables[i]; ++i) {
        CFCVariable *variable = variables[i];
        CFCType     *type     = CFCVariable_get_type(variable);
        const char  *name     = CFCVariable_get_name(variable);

        char *type_html;
        if (is_method && i == 0) {
            const char *prefix     = CFCClass_get_prefix(klass);
            const char *struct_sym = CFCClass_get_struct_sym(klass);
            const char *pattern    = "<span class=\"prefix\">%s</span>%s*";
            type_html = CFCUtil_sprintf(pattern, prefix, struct_sym);
        }
        else {
            type_html = S_type_to_html(klass, type);
        }

        result = CFCUtil_cat(result, "\n    ", type_html, " <strong>", name,
                             "</strong>", NULL);

        if (variables[i+1]) {
            result = CFCUtil_cat(result, ",", NULL);
        }
        if (CFCType_decremented(type)) {
            result = CFCUtil_cat(result,
                    " <span class=\"comment\">// decremented</span>", NULL);
        }

        FREEMEM(type_html);
    }

    result = CFCUtil_cat(result, "\n);\n", NULL);

    return result;
}

static char*
S_html_create_inheritance(CFCClass *klass) {
    CFCClass *ancestor = CFCClass_get_parent(klass);
    char     *result   = CFCUtil_strdup("");

    if (!ancestor) { return result; }

    const char *class_name = CFCClass_get_name(klass);
    result = CFCUtil_cat(result, "<h2>Inheritance</h2>\n<p>", class_name,
                         NULL);
    while (ancestor) {
        const char *ancestor_name = CFCClass_get_name(ancestor);
        const char *ancestor_sym  = CFCClass_full_struct_sym(ancestor);
        result = CFCUtil_cat(result, " is a <a href=\"", ancestor_sym,
                             ".html\">", ancestor_name, "</a>", NULL);
        ancestor = CFCClass_get_parent(ancestor);
    }
    result = CFCUtil_cat(result, ".</p>\n", NULL);

    return result;
}

static char*
S_md_to_html(CFCClass *klass, const char *md) {
    int options = CMARK_OPT_SMART
                  | CMARK_OPT_VALIDATE_UTF8
                  | CMARK_OPT_SAFE;
    cmark_node *doc = cmark_parse_document(md, strlen(md), options);
    S_convert_uris(klass, doc);
    char *html = cmark_render_html(doc, CMARK_OPT_DEFAULT);
    cmark_node_free(doc);

    return html;
}

static void
S_convert_uris(CFCClass *klass, cmark_node *node) {
    cmark_iter *iter = cmark_iter_new(node);
    cmark_event_type ev_type;

    while (CMARK_EVENT_DONE != (ev_type = cmark_iter_next(iter))) {
        cmark_node *cur = cmark_iter_get_node(iter);

        if (ev_type == CMARK_EVENT_EXIT
            && cmark_node_get_type(cur) == NODE_LINK
        ) {
            S_convert_uri(klass, cur);
        }
    }

    cmark_iter_free(iter);
}

static void
S_convert_uri(CFCClass *klass, cmark_node *link) {
    const char *uri = cmark_node_get_url(link);
    if (!uri || !CFCUri_is_clownfish_uri(uri)) {
        return;
    }

    char   *new_uri = NULL;
    CFCUri *uri_obj = CFCUri_new(uri, klass);
    int     type    = CFCUri_get_type(uri_obj);

    switch (type) {
        case CFC_URI_CLASS: {
            const char *struct_sym = CFCUri_full_struct_sym(uri_obj);
            new_uri = CFCUtil_sprintf("%s.html", struct_sym);
            break;
        }

        case CFC_URI_FUNCTION:
        case CFC_URI_METHOD: {
            const char *struct_sym = CFCUri_full_struct_sym(uri_obj);
            const char *func_sym   = CFCUri_get_func_sym(uri_obj);
            new_uri = CFCUtil_sprintf("%s.html#func_%s", struct_sym, func_sym);
            break;
        }
    }

    if (new_uri) {
        cmark_node_set_url(link, new_uri);

        if (!cmark_node_first_child(link)) {
            // Empty link text.
            char *link_text = CFCC_link_text(uri_obj, klass);

            if (link_text) {
                cmark_node *text_node = cmark_node_new(CMARK_NODE_TEXT);
                cmark_node_set_literal(text_node, link_text);
                cmark_node_append_child(link, text_node);
                FREEMEM(link_text);
            }
        }
    }
    else {
        // Remove link.
        cmark_node *child = cmark_node_first_child(link);
        while (child) {
            cmark_node *next = cmark_node_next(child);
            cmark_node_insert_before(link, child);
            child = next;
        }
        cmark_node_free(link);
    }

    CFCBase_decref((CFCBase*)uri_obj);
    FREEMEM(new_uri);
}

static char*
S_type_to_html(CFCClass *klass, CFCType *type) {
    const char *type_c = CFCType_to_c(type);

    if (CFCType_is_object(type)) {
        const char *struct_sym = CFCClass_full_struct_sym(klass);
        const char *specifier  = CFCType_get_specifier(type);
        const char *underscore = strchr(type_c, '_');

        if (underscore) {
            size_t  offset = underscore + 1 - type_c;
            char   *prefix = CFCUtil_strndup(specifier, offset);
            char   *retval;

            if (strcmp(specifier, struct_sym) == 0) {
                // Don't link types of the same class.
                retval = CFCUtil_sprintf("<span class=\"prefix\">%s</span>%s",
                                         prefix, type_c + offset);
            }
            else {
                const char *pattern =
                    "<span class=\"prefix\">%s</span>"
                    "<a href=\"%s.html\">"
                    "%s"
                    "</a>";
                retval = CFCUtil_sprintf(pattern, prefix, specifier,
                                         type_c + offset);
            }

            FREEMEM(prefix);
            return retval;
        }
    }

    return CFCUtil_strdup(type_c);
}


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

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCPerl.h"
#include "CFCParcel.h"
#include "CFCClass.h"
#include "CFCMethod.h"
#include "CFCHierarchy.h"
#include "CFCUtil.h"
#include "CFCPerlClass.h"
#include "CFCPerlSub.h"
#include "CFCPerlConstructor.h"
#include "CFCPerlMethod.h"
#include "CFCPerlTypeMap.h"
#include "CFCPerlPod.h"
#include "CFCBindCore.h"
#include "CFCDocument.h"

typedef struct CFCPerlPodFile {
    char *path;
    char *contents;
} CFCPerlPodFile;

struct CFCPerl {
    CFCBase base;
    CFCHierarchy *hierarchy;
    char *lib_dir;
    char *boot_class;
    char *header;
    char *footer;
    char *c_header;
    char *c_footer;
    char *pod_header;
    char *pod_footer;
    char *xs_path;
};

// Modify a string in place, swapping out "::" for the supplied character.
static void
S_replace_double_colons(char *text, char replacement);

static CFCPerlPodFile*
S_write_class_pod(CFCPerl *self);

static CFCPerlPodFile*
S_write_standalone_pod(CFCPerl *self);

static const CFCMeta CFCPERL_META = {
    "Clownfish::CFC::Binding::Perl",
    sizeof(CFCPerl),
    (CFCBase_destroy_t)CFCPerl_destroy
};

CFCPerl*
CFCPerl_new(CFCHierarchy *hierarchy, const char *lib_dir,
            const char *boot_class, const char *header, const char *footer) {
    CFCPerl *self = (CFCPerl*)CFCBase_allocate(&CFCPERL_META);
    return CFCPerl_init(self, hierarchy, lib_dir, boot_class, header, footer);
}

CFCPerl*
CFCPerl_init(CFCPerl *self, CFCHierarchy *hierarchy, const char *lib_dir,
             const char *boot_class, const char *header, const char *footer) {
    CFCUTIL_NULL_CHECK(hierarchy);
    CFCUTIL_NULL_CHECK(lib_dir);
    CFCUTIL_NULL_CHECK(boot_class);
    CFCUTIL_NULL_CHECK(header);
    CFCUTIL_NULL_CHECK(footer);
    self->hierarchy  = (CFCHierarchy*)CFCBase_incref((CFCBase*)hierarchy);
    self->lib_dir    = CFCUtil_strdup(lib_dir);
    self->boot_class = CFCUtil_strdup(boot_class);
    self->header     = CFCUtil_strdup(header);
    self->footer     = CFCUtil_strdup(footer);
    self->c_header   = CFCUtil_make_c_comment(header);
    self->c_footer   = CFCUtil_make_c_comment(footer);
    self->pod_header = CFCUtil_make_perl_comment(header);
    self->pod_footer = CFCUtil_make_perl_comment(footer);

    // Derive path to generated .xs file.
    self->xs_path = CFCUtil_sprintf("%s" CHY_DIR_SEP "%s.xs", lib_dir,
                                    boot_class);
    S_replace_double_colons(self->xs_path, CHY_DIR_SEP_CHAR);

    return self;
}

void
CFCPerl_destroy(CFCPerl *self) {
    CFCBase_decref((CFCBase*)self->hierarchy);
    FREEMEM(self->lib_dir);
    FREEMEM(self->boot_class);
    FREEMEM(self->header);
    FREEMEM(self->footer);
    FREEMEM(self->c_header);
    FREEMEM(self->c_footer);
    FREEMEM(self->pod_header);
    FREEMEM(self->pod_footer);
    FREEMEM(self->xs_path);
    CFCBase_destroy((CFCBase*)self);
}

static void
S_replace_double_colons(char *text, char replacement) {
    size_t pos = 0;
    for (char *ptr = text; *ptr != '\0'; ptr++) {
        if (strncmp(ptr, "::", 2) == 0) {
            text[pos++] = replacement;
            ptr++;
        }
        else {
            text[pos++] = *ptr;
        }
    }
    text[pos] = '\0';
}

char**
CFCPerl_write_pod(CFCPerl *self) {
    CFCPerlPodFile *class_pods      = S_write_class_pod(self);
    CFCPerlPodFile *standalone_pods = S_write_standalone_pod(self);

    size_t max_paths = 0;
    for (size_t i = 0; class_pods[i].contents; i++)      { max_paths++; }
    for (size_t i = 0; standalone_pods[i].contents; i++) { max_paths++; }
    char **pod_paths = (char**)CALLOCATE(max_paths + 1, sizeof(char*));

    // Write out any POD files that have changed.
    CFCPerlPodFile *file_arrays[2] = {
        class_pods,
        standalone_pods
    };
    size_t num_written = 0;
    for (size_t j = 0; j < 2; ++j) {
        CFCPerlPodFile *pod_files = file_arrays[j];

        for (size_t i = 0; pod_files[i].contents; i++) {
            char *pod      = pod_files[i].contents;
            char *pod_path = pod_files[i].path;
            char *pod_dir  = CFCUtil_strdup(pod_path);

            char *last_dir_sep = strrchr(pod_dir, CHY_DIR_SEP_CHAR);
            if (last_dir_sep) {
                *last_dir_sep = '\0';
                if (!CFCUtil_make_path(pod_dir)) {
                    CFCUtil_die("Can't make path %s", pod_dir);
                }
            }

            if (CFCUtil_write_if_changed(pod_path, pod, strlen(pod))) {
                pod_paths[num_written] = pod_path;
                num_written++;
            }
            else {
                FREEMEM(pod_path);
            }

            FREEMEM(pod);
            FREEMEM(pod_dir);
        }

        FREEMEM(pod_files);
    }
    pod_paths[num_written] = NULL;

    return pod_paths;
}

static CFCPerlPodFile*
S_write_class_pod(CFCPerl *self) {
    CFCPerlClass **registry  = CFCPerlClass_registry();
    size_t num_registered = 0;
    while (registry[num_registered] != NULL) { num_registered++; }
    CFCPerlPodFile *pod_files
        = (CFCPerlPodFile*)CALLOCATE(num_registered + 1,
                                     sizeof(CFCPerlPodFile));
    size_t count = 0;

    // Generate POD, but don't write.  That way, if there's an error while
    // generating pod, we leak memory but don't clutter up the file system.
    for (size_t i = 0; i < num_registered; i++) {
        const char *class_name = CFCPerlClass_get_class_name(registry[i]);
        char *raw_pod = CFCPerlClass_create_pod(registry[i]);
        if (!raw_pod) { continue; }
        char *pod = CFCUtil_sprintf("%s\n%s%s", self->pod_header, raw_pod,
                                    self->pod_footer);
        char *pod_path = CFCUtil_sprintf("%s" CHY_DIR_SEP "%s.pod",
                                         self->lib_dir, class_name);
        S_replace_double_colons(pod_path, CHY_DIR_SEP_CHAR);

        pod_files[count].contents = pod;
        pod_files[count].path     = pod_path;
        count++;

        FREEMEM(raw_pod);
    }
    pod_files[count].contents = NULL;
    pod_files[count].path     = NULL;

    return pod_files;
}

static CFCPerlPodFile*
S_write_standalone_pod(CFCPerl *self) {
    CFCDocument **docs = CFCDocument_get_registry();
    size_t num_pod_files = 0;
    while (docs[num_pod_files]) { num_pod_files++; }
    size_t alloc_size = (num_pod_files + 1) * sizeof(CFCPerlPodFile);
    CFCPerlPodFile *pod_files = (CFCPerlPodFile*)MALLOCATE(alloc_size);

    for (size_t i = 0; i < num_pod_files; i++) {
        CFCDocument *doc = docs[i];
        const char *path_part = CFCDocument_get_path_part(doc);
        char *module  = CFCUtil_global_replace(path_part, CHY_DIR_SEP, "::");
        char *md      = CFCDocument_get_contents(doc);
        char *raw_pod = CFCPerlPod_md_doc_to_pod(module, md);

        const char *pattern =
            "%s"
            "\n"
            "=encoding utf8\n"
            "\n"
            "%s"
            "%s";
        char *pod = CFCUtil_sprintf(pattern, self->pod_header, raw_pod,
                                    self->pod_footer);

        char *pod_path = CFCUtil_sprintf("%s" CHY_DIR_SEP "%s.pod",
                                         self->lib_dir, path_part);

        pod_files[i].contents = pod;
        pod_files[i].path     = pod_path;

        FREEMEM(raw_pod);
        FREEMEM(md);
        FREEMEM(module);
    }

    pod_files[num_pod_files].contents = NULL;
    pod_files[num_pod_files].path     = NULL;

    return pod_files;
}

static void
S_write_host_h(CFCPerl *self, CFCParcel *parcel) {
    const char *prefix = CFCParcel_get_prefix(parcel);
    const char *PREFIX = CFCParcel_get_PREFIX(parcel);

    char *guard = CFCUtil_sprintf("H_%sBOOT", PREFIX);

    const char pattern[] = 
        "%s\n"
        "\n"
        "#ifndef %s\n"
        "#define %s 1\n"
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n"
        "void\n"
        "%sbootstrap_perl(void);\n"
        "\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
        "\n"
        "#endif /* %s */\n"
        "\n"
        "%s\n";
    char *content
        = CFCUtil_sprintf(pattern, self->c_header, guard, guard, prefix, guard,
                          self->c_footer);

    const char *inc_dest = CFCHierarchy_get_include_dest(self->hierarchy);
    char *host_h_path = CFCUtil_sprintf("%s" CHY_DIR_SEP "%sperl.h", inc_dest,
                                        prefix);
    CFCUtil_write_file(host_h_path, content, strlen(content));
    FREEMEM(host_h_path);

    FREEMEM(content);
    FREEMEM(guard);
}

static void
S_write_host_c(CFCPerl *self, CFCParcel *parcel) {
    CFCClass **ordered = CFCHierarchy_ordered_classes(self->hierarchy);
    const char  *prefix     = CFCParcel_get_prefix(parcel);
    char        *includes   = CFCUtil_strdup("");
    char        *cb_defs    = CFCUtil_strdup("");
    char        *alias_adds = CFCUtil_strdup("");
    char        *isa_pushes = CFCUtil_strdup("");

    for (size_t i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        if (CFCClass_inert(klass)) { continue; }
        const char *class_prefix = CFCClass_get_prefix(klass);
        if (strcmp(class_prefix, prefix) != 0) { continue; }

        const char *class_name = CFCClass_get_name(klass);

        const char *include_h = CFCClass_include_h(klass);
        includes = CFCUtil_cat(includes, "#include \"", include_h,
                               "\"\n", NULL);

        // Callbacks.
        CFCMethod **fresh_methods = CFCClass_fresh_methods(klass);
        for (int meth_num = 0; fresh_methods[meth_num] != NULL; meth_num++) {
            CFCMethod *method = fresh_methods[meth_num];

            // Define callback.
            if (CFCMethod_novel(method) && !CFCMethod_final(method)) {
                char *cb_def = CFCPerlMethod_callback_def(method, klass);
                cb_defs = CFCUtil_cat(cb_defs, cb_def, "\n", NULL);
                FREEMEM(cb_def);
            }
        }

        // Add class aliases.
        CFCPerlClass *class_binding = CFCPerlClass_singleton(class_name);
        if (class_binding) {
            const char *class_var = CFCClass_full_class_var(klass);
            const char **aliases
                = CFCPerlClass_get_class_aliases(class_binding);
            for (size_t j = 0; aliases[j] != NULL; j++) {
                const char *alias = aliases[j];
                int alias_len  = (int)strlen(alias);
                const char pattern[] =
                    "    cfish_Class_add_alias_to_registry("
                    "%s, \"%s\", %d);\n";
                char *alias_add
                    = CFCUtil_sprintf(pattern, class_var, alias, alias_len);
                alias_adds = CFCUtil_cat(alias_adds, alias_add, NULL);
                FREEMEM(alias_add);
            }

            char *metadata_code
                = CFCPerlClass_method_metadata_code(class_binding);
            alias_adds = CFCUtil_cat(alias_adds, metadata_code, NULL);
            FREEMEM(metadata_code);
        }

        CFCClass *parent = CFCClass_get_parent(klass);
        if (parent) {
            const char *parent_class_name = CFCClass_get_name(parent);
            isa_pushes
                = CFCUtil_cat(isa_pushes, "    isa = get_av(\"",
                              class_name, "::ISA\", 1);\n", NULL);
            isa_pushes
                = CFCUtil_cat(isa_pushes, "    av_push(isa, newSVpv(\"", 
                              parent_class_name, "\", 0));\n", NULL);
        }
    }

    const char pattern[] =
        "%s"
        "\n"
        "#include \"%sperl.h\"\n"
        "#include \"XSBind.h\"\n"
        "#include \"Clownfish/Class.h\"\n"
        "#include \"Clownfish/Err.h\"\n"
        "#include \"Clownfish/Obj.h\"\n"
        "%s"
        "\n"
        "/* Avoid conflicts with Clownfish bool type. */\n"
        "#define HAS_BOOL\n"
        "#define PERL_NO_GET_CONTEXT\n"
        "#include \"EXTERN.h\"\n"
        "#include \"perl.h\"\n"
        "#include \"XSUB.h\"\n"
        "\n"
        "static void\n"
        "S_finish_callback_void(pTHX_ const char *meth_name) {\n"
        "    int count = call_method(meth_name, G_VOID | G_DISCARD);\n"
        "    if (count != 0) {\n"
        "        CFISH_THROW(CFISH_ERR, \"Bad callback to '%%s': %%i32\",\n"
        "                    meth_name, (int32_t)count);\n"
        "    }\n"
        "    FREETMPS;\n"
        "    LEAVE;\n"
        "}\n"
        "\n"
        "static CFISH_INLINE SV*\n"
        "SI_do_callback_sv(pTHX_ const char *meth_name) {\n"
        "    int count = call_method(meth_name, G_SCALAR);\n"
        "    if (count != 1) {\n"
        "        CFISH_THROW(CFISH_ERR, \"Bad callback to '%%s': %%i32\",\n"
        "                    meth_name, (int32_t)count);\n"
        "    }\n"
        "    dSP;\n"
        "    SV *return_sv = POPs;\n"
        "    PUTBACK;\n"
        "    return return_sv;\n"
        "}\n"
        "\n"
        "static int64_t\n"
        "S_finish_callback_i64(pTHX_ const char *meth_name) {\n"
        "    SV *return_sv = SI_do_callback_sv(aTHX_ meth_name);\n"
        "    int64_t retval;\n"
        "    if (sizeof(IV) == 8) {\n"
        "        retval = (int64_t)SvIV(return_sv);\n"
        "    }\n"
        "    else {\n"
        "        if (SvIOK(return_sv)) {\n"
        "            // It's already no more than 32 bits, so don't convert.\n"
        "            retval = SvIV(return_sv);\n"
        "        }\n"
        "        else {\n"
        "            // Maybe lossy.\n"
        "            double temp = SvNV(return_sv);\n"
        "            retval = (int64_t)temp;\n"
        "        }\n"
        "    }\n"
        "    FREETMPS;\n"
        "    LEAVE;\n"
        "    return retval;\n"
        "}\n"
        "\n"
        "static double\n"
        "S_finish_callback_f64(pTHX_ const char *meth_name) {\n"
        "    SV *return_sv = SI_do_callback_sv(aTHX_ meth_name);\n"
        "    double retval = SvNV(return_sv);\n"
        "    FREETMPS;\n"
        "    LEAVE;\n"
        "    return retval;\n"
        "}\n"
        "\n"
        "static cfish_Obj*\n"
        "S_finish_callback_obj(pTHX_ void *vself, const char *meth_name,\n"
        "                      int nullable) {\n"
        "    SV *return_sv = SI_do_callback_sv(aTHX_ meth_name);\n"
        "    cfish_Obj *retval\n"
        "        = XSBind_perl_to_cfish_nullable(aTHX_ return_sv, CFISH_OBJ);\n"
        "    FREETMPS;\n"
        "    LEAVE;\n"
        "    if (!nullable && !retval) {\n"
        "        CFISH_THROW(CFISH_ERR, \"%%o#%%s cannot return NULL\",\n"
        "                    cfish_Obj_get_class_name((cfish_Obj*)vself),\n"
        "                    meth_name);\n"
        "    }\n"
        "    return retval;\n"
        "}\n"
	"\n"
        "%s"
        "\n"
        "void\n"
        "%sbootstrap_perl() {\n"
        "    dTHX;\n"
        "    %sbootstrap_parcel();\n"
        "\n"
        "%s"
        "\n"
        "    AV *isa;\n"
        "%s"
        "}\n"
        "\n"
        "%s";
    char *content
        = CFCUtil_sprintf(pattern, self->c_header, prefix, includes, cb_defs,
                          prefix, prefix, alias_adds, isa_pushes,
                          self->c_footer);

    const char *src_dest = CFCHierarchy_get_source_dest(self->hierarchy);
    char *host_c_path = CFCUtil_sprintf("%s" CHY_DIR_SEP "%sperl.c", src_dest,
                                        prefix);
    CFCUtil_write_file(host_c_path, content, strlen(content));
    FREEMEM(host_c_path);

    FREEMEM(content);
    FREEMEM(isa_pushes);
    FREEMEM(alias_adds);
    FREEMEM(cb_defs);
    FREEMEM(includes);
    FREEMEM(ordered);
}

void
CFCPerl_write_hostdefs(CFCPerl *self) {
    const char pattern[] =
        "%s\n"
        "\n"
        "#ifndef H_CFISH_HOSTDEFS\n"
        "#define H_CFISH_HOSTDEFS 1\n"
        "\n"
        "/* Refcount / host object */\n"
        "typedef union {\n"
        "    size_t  count;\n"
        "    void   *host_obj;\n"
        "} cfish_ref_t;\n"
        "\n"
        "#define CFISH_OBJ_HEAD\\\n"
        "   cfish_ref_t ref;\n"
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

void
CFCPerl_write_host_code(CFCPerl *self) {
    CFCParcel **parcels = CFCParcel_all_parcels();

    for (size_t i = 0; parcels[i]; ++i) {
        CFCParcel *parcel = parcels[i];

        if (!CFCParcel_included(parcel)) {
            S_write_host_h(self, parcel);
            S_write_host_c(self, parcel);
        }
    }
}

static char*
S_xs_file_contents(CFCPerl *self, const char *generated_xs,
                   const char *xs_init, const char *hand_rolled_xs) {
    char *bootstrap_calls = CFCUtil_strdup("");
    CFCParcel **parcels   = CFCParcel_all_parcels();
    for (size_t i = 0; parcels[i]; ++i) {
        if (!CFCParcel_included(parcels[i])) {
            const char *prefix = CFCParcel_get_prefix(parcels[i]);
            bootstrap_calls
                = CFCUtil_cat(bootstrap_calls, "    ", prefix,
                              "bootstrap_perl();\n", NULL);
        }
    }

    const char pattern[] =
        "%s"
        "\n"
        "%s\n"
        "\n"
        "MODULE = %s   PACKAGE = %s\n"
        "\n"
        "BOOT:\n"
        "{\n"
        "    const char* file = __FILE__;\n"
        "%s"
        "%s"
        "}\n"
        "\n"
        "%s\n"
        "\n"
        "%s";
    char *contents
        = CFCUtil_sprintf(pattern, self->c_header, generated_xs,
                          self->boot_class, self->boot_class, bootstrap_calls,
                          xs_init, hand_rolled_xs, self->c_footer);

    FREEMEM(bootstrap_calls);
    return contents;
}

static char*
S_add_xs_init(char *xs_init, CFCPerlSub *xsub) {
    const char *c_name = CFCPerlSub_c_name(xsub);
    const char *perl_name = CFCPerlSub_perl_name(xsub);
    xs_init = CFCUtil_cat(xs_init, "    newXS(\"", perl_name, "\", ", c_name,
                          ", file);\n", NULL);
    return xs_init;
}

void
CFCPerl_write_bindings(CFCPerl *self) {
    CFCParcel    **parcels  = CFCParcel_all_parcels();
    CFCClass     **ordered  = CFCHierarchy_ordered_classes(self->hierarchy);
    CFCPerlClass **registry = CFCPerlClass_registry();
    char *hand_rolled_xs   = CFCUtil_strdup("");
    char *generated_xs     = CFCUtil_strdup("");
    char *xs_init          = CFCUtil_strdup("");

    // Bake the parcel privacy defines into the XS, so it can be compiled
    // without any extra compiler flags.
    for (size_t i = 0; parcels[i]; ++i) {
        if (!CFCParcel_included(parcels[i])) {
            const char *privacy_sym = CFCParcel_get_privacy_sym(parcels[i]);
            generated_xs = CFCUtil_cat(generated_xs, "#define ", privacy_sym,
                                       "\n", NULL);
        }
    }
    generated_xs = CFCUtil_cat(generated_xs, "\n", NULL);

    // Include XSBind.h and *_perl.h.
    generated_xs = CFCUtil_cat(generated_xs, "#include \"XSBind.h\"\n", NULL);
    for (size_t i = 0; parcels[i]; ++i) {
        if (!CFCParcel_included(parcels[i])) {
            const char *prefix = CFCParcel_get_prefix(parcels[i]);
            generated_xs = CFCUtil_cat(generated_xs, "#include \"", prefix,
                                       "perl.h\"\n", NULL);
        }
    }

    // Pound-includes for generated headers.
    for (size_t i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        if (CFCClass_included(klass)) { continue; }
        const char *include_h = CFCClass_include_h(klass);
        generated_xs = CFCUtil_cat(generated_xs, "#include \"", include_h,
                                   "\"\n", NULL);
    }
    generated_xs = CFCUtil_cat(generated_xs, "\n", NULL);

    for (size_t i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        if (CFCClass_included(klass)) { continue; }

        // Constructors.
        CFCPerlConstructor **constructors
            = CFCPerlClass_constructor_bindings(klass);
        for (size_t j = 0; constructors[j] != NULL; j++) {
            CFCPerlSub *xsub = (CFCPerlSub*)constructors[j];

            // Add the XSUB function definition.
            char *xsub_def
                = CFCPerlConstructor_xsub_def(constructors[j], klass);
            generated_xs = CFCUtil_cat(generated_xs, xsub_def, "\n",
                                       NULL);
            FREEMEM(xsub_def);

            // Add XSUB initialization at boot.
            xs_init = S_add_xs_init(xs_init, xsub);

            CFCBase_decref((CFCBase*)constructors[j]);
        }
        FREEMEM(constructors);

        // Methods.
        CFCPerlMethod **methods = CFCPerlClass_method_bindings(klass);
        for (size_t j = 0; methods[j] != NULL; j++) {
            CFCPerlSub *xsub = (CFCPerlSub*)methods[j];

            // Add the XSUB function definition.
            char *xsub_def = CFCPerlMethod_xsub_def(methods[j], klass);
            generated_xs = CFCUtil_cat(generated_xs, xsub_def, "\n",
                                       NULL);
            FREEMEM(xsub_def);

            // Add XSUB initialization at boot.
            xs_init = S_add_xs_init(xs_init, xsub);

            CFCBase_decref((CFCBase*)methods[j]);
        }
        FREEMEM(methods);
    }

    // Hand-rolled XS.
    for (size_t i = 0; registry[i] != NULL; i++) {
        const char *xs = CFCPerlClass_get_xs_code(registry[i]);
        hand_rolled_xs = CFCUtil_cat(hand_rolled_xs, xs, "\n", NULL);
    }

    // Write out if there have been any changes.
    char *xs_file_contents
        = S_xs_file_contents(self, generated_xs, xs_init, hand_rolled_xs);
    CFCUtil_write_if_changed(self->xs_path, xs_file_contents,
                             strlen(xs_file_contents));

    FREEMEM(xs_file_contents);
    FREEMEM(hand_rolled_xs);
    FREEMEM(xs_init);
    FREEMEM(generated_xs);
    FREEMEM(ordered);
}

void
CFCPerl_write_xs_typemap(CFCPerl *self) {
    CFCPerlTypeMap_write_xs_typemap(self->hierarchy);
}


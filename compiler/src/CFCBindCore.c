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
#include "CFCBindCore.h"
#include "CFCBindClass.h"
#include "CFCBindFile.h"
#include "CFCClass.h"
#include "CFCFile.h"
#include "CFCHierarchy.h"
#include "CFCParcel.h"
#include "CFCUtil.h"

#define STRING(s)  #s
#define XSTRING(s) STRING(s)

struct CFCBindCore {
    CFCBase base;
    CFCHierarchy *hierarchy;
    char         *header;
    char         *footer;
};

/* Write the "parcel.h" header file, which contains common symbols needed by
 * all classes, plus typedefs for all class structs.
 */
static void
S_write_parcel_h(CFCBindCore *self, CFCParcel *parcel);

/* Write the "parcel.c" file containing autogenerated implementation code.
 */
static void
S_write_parcel_c(CFCBindCore *self, CFCParcel *parcel);

static char*
S_charmony_defines();

static char*
S_charmony_stdbool_defines();

char*
S_charmony_stdint_defines();

static const CFCMeta CFCBINDCORE_META = {
    "Clownfish::CFC::Binding::Core",
    sizeof(CFCBindCore),
    (CFCBase_destroy_t)CFCBindCore_destroy
};

CFCBindCore*
CFCBindCore_new(CFCHierarchy *hierarchy, const char *header,
                const char *footer) {
    CFCBindCore *self = (CFCBindCore*)CFCBase_allocate(&CFCBINDCORE_META);
    return CFCBindCore_init(self, hierarchy, header, footer);
}

CFCBindCore*
CFCBindCore_init(CFCBindCore *self, CFCHierarchy *hierarchy,
                 const char *header, const char *footer) {
    CFCUTIL_NULL_CHECK(hierarchy);
    CFCUTIL_NULL_CHECK(header);
    CFCUTIL_NULL_CHECK(footer);
    self->hierarchy = (CFCHierarchy*)CFCBase_incref((CFCBase*)hierarchy);
    self->header    = CFCUtil_strdup(header);
    self->footer    = CFCUtil_strdup(footer);
    return self;
}

void
CFCBindCore_destroy(CFCBindCore *self) {
    CFCBase_decref((CFCBase*)self->hierarchy);
    FREEMEM(self->header);
    FREEMEM(self->footer);
    CFCBase_destroy((CFCBase*)self);
}

int
CFCBindCore_write_all_modified(CFCBindCore *self, int modified) {
    CFCHierarchy *hierarchy = self->hierarchy;
    const char   *header    = self->header;
    const char   *footer    = self->footer;

    // Discover whether files need to be regenerated.
    modified = CFCHierarchy_propagate_modified(hierarchy, modified);

    // Iterate over all File objects, writing out those which don't have
    // up-to-date auto-generated files.
    const char *inc_dest = CFCHierarchy_get_include_dest(hierarchy);
    CFCFile **files = CFCHierarchy_files(hierarchy);
    for (int i = 0; files[i] != NULL; i++) {
        if (CFCFile_get_modified(files[i])) {
            CFCBindFile_write_h(files[i], inc_dest, header, footer);
        }
    }

    // If any class definition has changed, rewrite the parcel.h and parcel.c
    // files.
    if (modified) {
        CFCParcel **parcels = CFCParcel_all_parcels();
        for (size_t i = 0; parcels[i]; ++i) {
            CFCParcel *parcel = parcels[i];
            // TODO: Skip parcels the source parcels don't depend on.
            S_write_parcel_h(self, parcel);
            if (!CFCParcel_included(parcel)) {
                S_write_parcel_c(self, parcel);
            }
        }
        FREEMEM(parcels);
    }

    return modified;
}

/* Write the "parcel.h" header file, which contains common symbols needed by
 * all classes, plus typedefs for all class structs.
 */
static void
S_write_parcel_h(CFCBindCore *self, CFCParcel *parcel) {
    CFCHierarchy *hierarchy   = self->hierarchy;
    const char   *prefix      = CFCParcel_get_prefix(parcel);
    const char   *PREFIX      = CFCParcel_get_PREFIX(parcel);
    const char   *privacy_sym = CFCParcel_get_privacy_sym(parcel);

    char *charmony_defines = S_charmony_defines();

    // Declare object structs for all instantiable classes.
    char *typedefs = CFCUtil_strdup("");
    CFCClass **ordered = CFCHierarchy_ordered_classes(hierarchy);
    for (int i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        const char *class_prefix = CFCClass_get_prefix(klass);
        if (strcmp(class_prefix, prefix) != 0) { continue; }

        if (!CFCClass_inert(klass)) {
            const char *full_struct = CFCClass_full_struct_sym(klass);
            typedefs = CFCUtil_cat(typedefs, "typedef struct ", full_struct,
                                   " ", full_struct, ";\n", NULL);
        }
    }
    FREEMEM(ordered);

    // Special includes for Clownfish parcel.
    const char *cfish_includes_pattern =
        "#include <stdarg.h>\n"
        "#include <stddef.h>\n"
        "\n"
        "%s"
        "\n"
        "#include \"cfish_hostdefs.h\"\n";

    // Special definitions for Clownfish parcel.
    const char *cfish_defs =
        "/* Generic method pointer.\n"
        " */\n"
        "typedef void\n"
        "(*cfish_method_t)(const void *vself);\n"
        "\n"
        "/* Access the function pointer for a given method from the vtable.\n"
        " */\n"
        "#define CFISH_METHOD_PTR(_vtable, _full_meth) \\\n"
        "     ((_full_meth ## _t)cfish_method(_vtable, _full_meth ## _OFFSET))\n"
        "\n"
        "static CFISH_INLINE cfish_method_t\n"
        "cfish_method(const void *vtable, size_t offset) {\n"
        "    union { char *cptr; cfish_method_t *fptr; } ptr;\n"
        "    ptr.cptr = (char*)vtable + offset;\n"
        "    return ptr.fptr[0];\n"
        "}\n"
        "\n"
        "typedef struct cfish_Dummy {\n"
        "   CFISH_OBJ_HEAD\n"
        "   void *vtable;\n"
        "} cfish_Dummy;\n"
        "\n"
        "/* Access the function pointer for a given method from the object.\n"
        " */\n"
        "static CFISH_INLINE cfish_method_t\n"
        "cfish_obj_method(const void *object, size_t offset) {\n"
        "    cfish_Dummy *dummy = (cfish_Dummy*)object;\n"
        "    return cfish_method(dummy->vtable, offset);\n"
        "}\n"
        "\n"
        "/* Access the function pointer for the given method in the superclass's\n"
        " * vtable. */\n"
        "#define CFISH_SUPER_METHOD_PTR(_vtable, _full_meth) \\\n"
        "     ((_full_meth ## _t)cfish_super_method(_vtable, \\\n"
        "                                           _full_meth ## _OFFSET))\n"
        "\n"
        "extern CFISH_VISIBLE size_t cfish_VTable_offset_of_parent;\n"
        "static CFISH_INLINE cfish_method_t\n"
        "cfish_super_method(const void *vtable, size_t offset) {\n"
        "    char *vt_as_char = (char*)vtable;\n"
        "    cfish_VTable **parent_ptr\n"
        "        = (cfish_VTable**)(vt_as_char + cfish_VTable_offset_of_parent);\n"
        "    return cfish_method(*parent_ptr, offset);\n"
        "}\n"
        "\n"
        "/* Return a boolean indicating whether a method has been overridden.\n"
        " */\n"
        "#define CFISH_OVERRIDDEN(_self, _full_meth, _full_func) \\\n"
        "    (cfish_obj_method(_self, _full_meth ## _OFFSET )\\\n"
        "        != (cfish_method_t)_full_func)\n"
        "\n"
        "/* Structs for VTable initialization.\n"
        " */\n"
        "\n"
        "typedef struct cfish_MethodSpec {\n"
        "    int             is_novel;\n"
        "    const char     *name;\n"
        "    cfish_method_t  func;\n"
        "    cfish_method_t  callback_func;\n"
        "    size_t         *offset;\n"
        "} cfish_MethodSpec;\n"
        "\n"
        "typedef struct cfish_VTableSpec {\n"
        "    cfish_VTable     **vtable;\n"
        "    cfish_VTable     **parent;\n"
        "    const char        *name;\n"
        "    size_t             ivars_size;\n"
        "    size_t            *ivars_offset_ptr;\n"
        "    size_t             num_fresh;\n"
        "    size_t             num_novel;\n"
        "    cfish_MethodSpec  *method_specs;\n"
        "} cfish_VTableSpec;\n"
        "\n"
        "#ifdef CFISH_USE_SHORT_NAMES\n"
        "  #define METHOD_PTR               CFISH_METHOD_PTR\n"
        "  #define SUPER_METHOD_PTR         CFISH_SUPER_METHOD_PTR\n"
        "  #define OVERRIDDEN               CFISH_OVERRIDDEN\n"
        "  #define MethodSpec               cfish_MethodSpec\n"
        "  #define VTableSpec               cfish_VTableSpec\n"
        "#endif\n"
        "\n";

    const char *extra_defs;
    char *extra_includes;
    if (strcmp(prefix, "cfish_") == 0) {
        extra_defs = cfish_defs;
        extra_includes = CFCUtil_sprintf(cfish_includes_pattern,
                                         charmony_defines);
    }
    else {
        extra_defs = "";
        extra_includes = CFCUtil_strdup("");

        // Include parcel.h of dependent parcels.
        CFCParcel **dep_parcels = CFCParcel_dependent_parcels(parcel);
        for (size_t i = 0; dep_parcels[i]; ++i) {
            const char *dep_prefix = CFCParcel_get_prefix(dep_parcels[i]);
            extra_includes = CFCUtil_cat(extra_includes, "#include <",
                                         dep_prefix, "parcel.h>\n", NULL);
        }
        FREEMEM(dep_parcels);
    }

    const char pattern[] =
        "%s\n"
        "#ifndef CFISH_%sPARCEL_H\n"
        "#define CFISH_%sPARCEL_H 1\n"
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n"
        "%s" // Extra includes.
        "\n"
        "#ifdef %s\n"
        "  #include \"charmony.h\"\n"
        "  #define %sVISIBLE CFISH_EXPORT\n"
        "#else\n"
        "  #define %sVISIBLE CFISH_IMPORT\n"
        "#endif\n"
        "\n"
        "%s" // Typedefs.
        "\n"
        "%s" // Extra definitions.
        "%sVISIBLE void\n"
        "%sbootstrap_inheritance();\n"
        "\n"
        "%sVISIBLE void\n"
        "%sbootstrap_parcel();\n"
        "\n"
        "void\n"
        "%sinit_parcel();\n"
        "\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
        "\n"
        "#endif /* CFISH_%sPARCEL_H */\n"
        "\n"
        "%s\n"
        "\n";
    char *file_content
        = CFCUtil_sprintf(pattern, self->header, PREFIX, PREFIX,
                          extra_includes, privacy_sym, PREFIX, PREFIX,
                          typedefs, extra_defs, PREFIX, prefix, PREFIX, prefix,
                          prefix, PREFIX, self->footer);

    // Unlink then write file.
    const char *inc_dest = CFCHierarchy_get_include_dest(hierarchy);
    char *filepath = CFCUtil_sprintf("%s" CHY_DIR_SEP "%sparcel.h", inc_dest,
                                     prefix);
    remove(filepath);
    CFCUtil_write_file(filepath, file_content, strlen(file_content));
    FREEMEM(filepath);

    FREEMEM(charmony_defines);
    FREEMEM(typedefs);
    FREEMEM(extra_includes);
    FREEMEM(file_content);
}

static void
S_write_parcel_c(CFCBindCore *self, CFCParcel *parcel) {
    CFCHierarchy *hierarchy = self->hierarchy;
    const char   *prefix    = CFCParcel_get_prefix(parcel);

    // Aggregate C code for the parcel.
    char *privacy_syms = CFCUtil_strdup("");
    char *includes     = CFCUtil_strdup("");
    char *c_data       = CFCUtil_strdup("");
    char *vt_specs = CFCUtil_strdup(
        "static cfish_VTableSpec vtable_specs[] = {\n");
    int num_specs = 0;
    CFCClass **ordered  = CFCHierarchy_ordered_classes(hierarchy);
    for (int i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];
        const char *class_prefix = CFCClass_get_prefix(klass);
        if (strcmp(class_prefix, prefix) != 0) { continue; }

        const char *include_h = CFCClass_include_h(klass);
        includes = CFCUtil_cat(includes, "#include \"", include_h,
                               "\"\n", NULL);

        CFCBindClass *class_binding = CFCBindClass_new(klass);
        char *class_c_data = CFCBindClass_to_c_data(class_binding);
        c_data = CFCUtil_cat(c_data, class_c_data, "\n", NULL);
        FREEMEM(class_c_data);
        if (!CFCClass_inert(klass)) {
            if (num_specs != 0) {
                vt_specs = CFCUtil_cat(vt_specs, ",\n", NULL);
            }
            char *vt_spec = CFCBindClass_spec_def(class_binding);
            vt_specs = CFCUtil_cat(vt_specs, vt_spec, NULL);
            FREEMEM(vt_spec);
            ++num_specs;
        }
        CFCBase_decref((CFCBase*)class_binding);
        const char *privacy_sym = CFCClass_privacy_symbol(klass);
        privacy_syms = CFCUtil_cat(privacy_syms, "#define ",
                                   privacy_sym, "\n", NULL);
    }
    vt_specs = CFCUtil_cat(vt_specs, "\n};\n", NULL);
    FREEMEM(ordered);

    // Bootstrapping code for dependent parcels.
    //
    // bootstrap_inheritance() first calls bootstrap_inheritance() for all
    // parcels from which classes are inherited. Then the VTables of the parcel
    // are initialized. It aborts on recursive invocation.
    //
    // bootstrap_parcel() first calls bootstrap_inheritance() of its own
    // parcel. Then it calls bootstrap_parcel() for all dependent parcels.
    // Finally, it calls init_parcel(). Recursive invocation is allowed.

    char *inh_bootstrap = CFCUtil_strdup("");
    char *dep_bootstrap = CFCUtil_strdup("");
    CFCParcel **inh_parcels = CFCParcel_inherited_parcels(parcel);
    for (size_t i = 0; inh_parcels[i]; ++i) {
        const char *inh_prefix = CFCParcel_get_prefix(inh_parcels[i]);
        inh_bootstrap = CFCUtil_cat(inh_bootstrap, "    ", inh_prefix,
                                    "bootstrap_inheritance();\n", NULL);
    }
    FREEMEM(inh_parcels);
    CFCParcel **dep_parcels = CFCParcel_dependent_parcels(parcel);
    for (size_t i = 0; dep_parcels[i]; ++i) {
        const char *dep_prefix = CFCParcel_get_prefix(dep_parcels[i]);
        dep_bootstrap = CFCUtil_cat(dep_bootstrap, "    ", dep_prefix,
                                    "bootstrap_parcel();\n", NULL);
    }
    FREEMEM(dep_parcels);

    char pattern[] =
        "%s\n"
        "\n"
        "#define C_CFISH_VTABLE\n"          // Needed for method_ptrs offset.
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "%s\n"
        "#include \"%sparcel.h\"\n"
        "#include \"callbacks.h\"\n"
        "#include \"Clownfish/CharBuf.h\"\n" // Needed for dump/load.
        "#include \"Clownfish/Err.h\"\n"     // Needed for dump/load.
        "#include \"Clownfish/Num.h\"\n"     // Needed for dump/load.
        "#include \"Clownfish/VArray.h\"\n"  // Needed for dump/load.
        "#include \"Clownfish/VTable.h\"\n"  // Needed for method_ptrs offset.
        "%s\n"
        "\n"
        "%s\n"
        "\n"
        "/* VTableSpec structs for initialization.\n"
        " */\n"
        "%s\n"
        "\n"
        "static int bootstrap_state = 0;\n"
        "\n"
        "void\n"
        "%sbootstrap_inheritance() {\n"
        "    if (bootstrap_state == 1) {\n"
        "        fprintf(stderr, \"Cycle in class inheritance between\"\n"
        "                        \" parcels detected.\\n\");\n"
        "        abort();\n"
        "    }\n"
        "    if (bootstrap_state >= 2) { return; }\n"
        "    bootstrap_state = 1;\n"
        "%s" // Bootstrap inherited parcels.
        "    cfish_VTable_bootstrap(vtable_specs, %d);\n"
        "    bootstrap_state = 2;\n"
        "}\n"
        "\n"
        "void\n"
        "%sbootstrap_parcel() {\n"
        "    if (bootstrap_state >= 3) { return; }\n"
        "    %sbootstrap_inheritance();\n"
        "    bootstrap_state = 3;\n"
        "%s" // Finish bootstrapping of all dependent parcels.
        "    %sinit_parcel();\n"
        "}\n"
        "\n"
        "%s\n";
    char *file_content
        = CFCUtil_sprintf(pattern, self->header, privacy_syms, prefix,
                          includes, c_data, vt_specs, prefix, inh_bootstrap,
                          num_specs, prefix, prefix, dep_bootstrap, prefix,
                          self->footer);

    // Unlink then open file.
    const char *src_dest = CFCHierarchy_get_source_dest(hierarchy);
    char *filepath = CFCUtil_sprintf("%s" CHY_DIR_SEP "%sparcel.c", src_dest,
                                     prefix);
    remove(filepath);
    CFCUtil_write_file(filepath, file_content, strlen(file_content));
    FREEMEM(filepath);

    FREEMEM(privacy_syms);
    FREEMEM(includes);
    FREEMEM(c_data);
    FREEMEM(vt_specs);
    FREEMEM(inh_bootstrap);
    FREEMEM(dep_bootstrap);
    FREEMEM(file_content);
}

/* Write the "callbacks.h" header file, which contains declarations of host
 * callbacks.
 */
void
CFCBindCore_write_callbacks_h(CFCBindCore *self) {
    CFCHierarchy  *hierarchy   = self->hierarchy;
    CFCClass     **ordered     = CFCHierarchy_ordered_classes(hierarchy);
    char          *includes    = CFCUtil_strdup("");
    char          *all_cb_decs = CFCUtil_strdup("");

    for (int i = 0; ordered[i] != NULL; i++) {
        CFCClass *klass = ordered[i];

        const char *include_h = CFCClass_include_h(klass);
        includes = CFCUtil_cat(includes, "#include \"", include_h, "\"\n",
                               NULL);

        if (!CFCClass_included(klass)) {
            CFCBindClass *class_binding = CFCBindClass_new(klass);
            char *cb_decs = CFCBindClass_callback_decs(class_binding);
            all_cb_decs = CFCUtil_cat(all_cb_decs, cb_decs, NULL);
            FREEMEM(cb_decs);
            CFCBase_decref((CFCBase*)class_binding);
        }
    }

    FREEMEM(ordered);

    const char pattern[] =
        "%s\n"
        "#ifndef CFCCALLBACKS_H\n"
        "#define CFCCALLBACKS_H 1\n"
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n"
        "%s"
        "\n"
        "%s"
        "\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
        "\n"
        "#endif /* CFCCALLBACKS_H */\n"
        "\n"
        "%s\n"
        "\n";
    char *file_content
        = CFCUtil_sprintf(pattern, self->header, includes, all_cb_decs,
                          self->footer);

    // Unlink then write file.
    const char *inc_dest = CFCHierarchy_get_include_dest(hierarchy);
    char *filepath = CFCUtil_sprintf("%s" CHY_DIR_SEP "callbacks.h", inc_dest);
    remove(filepath);
    CFCUtil_write_file(filepath, file_content, strlen(file_content));
    FREEMEM(filepath);

    FREEMEM(includes);
    FREEMEM(all_cb_decs);
    FREEMEM(file_content);
}

static char*
S_charmony_defines() {
    char *stdbool_defs = S_charmony_stdbool_defines();
    char *stdint_defs  = S_charmony_stdint_defines();

    const char *pattern =
        "#define CFISH_INLINE %s\n"
        "#define CFISH_EXPORT %s\n"
        "#define CFISH_IMPORT %s\n"
        "\n"
        "%s"
        "%s";
    char *defines
        = CFCUtil_sprintf(pattern, XSTRING(CHY_INLINE), XSTRING(CHY_EXPORT),
                          XSTRING(CHY_IMPORT), stdbool_defs, stdint_defs);

    FREEMEM(stdbool_defs);
    FREEMEM(stdint_defs);
    return defines;
}

static char*
S_charmony_stdbool_defines() {
#ifdef CHY_HAS_STDBOOL_H
    const char *defines = "#include <stdbool.h>\n";
#else
    const char *defines =
        "#if (!defined(__cplusplus) && !defined(CFISH_HAS_STDBOOL))\n"
        "  typedef int bool;\n"
        "  #ifndef true\n"
        "    #define true 1\n"
        "  #endif\n"
        "  #ifndef false\n"
        "    #define false 0\n"
        "  #endif\n"
        "#endif\n";
#endif

    return CFCUtil_strdup(defines);
}

char*
S_charmony_stdint_defines() {
#ifdef CHY_HAS_STDINT_H
    return CFCUtil_strdup("#include <stdint.h>\n");
#else
    const char *pattern =
        "#ifndef CFISH_HAS_STDINT\n"
        "  typedef %s int8_t;\n"
        "  typedef %s uint8_t;\n"
        "  typedef %s int16_t;\n"
        "  typedef %s uint16_t;\n"
        "  typedef %s int32_t;\n"
        "  typedef %s uint32_t;\n"
        "  typedef %s int64_t;\n"
        "  typedef %s uint64_t;\n"
        "#endif\n";
    return CFCUtil_sprintf(pattern,
                           XSTRING(CHY_INT8_T),  XSTRING(CHY_UINT8_T),
                           XSTRING(CHY_INT16_T), XSTRING(CHY_UINT16_T),
                           XSTRING(CHY_INT32_T), XSTRING(CHY_UINT32_T),
                           XSTRING(CHY_INT64_T), XSTRING(CHY_UINT64_T));
#endif
}



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

#include "CFCBindSpecs.h"

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCClass.h"
#include "CFCMethod.h"
#include "CFCParcel.h"
#include "CFCUtil.h"

struct CFCBindSpecs {
    CFCBase base;

    char *novel_specs;
    char *overridden_specs;
    char *inherited_specs;
    char *class_specs;
    char *init_code;

    int num_novel;
    int num_overridden;
    int num_inherited;
    int num_specs;
};

static char*
S_ivars_size(CFCClass *klass);

static char*
S_novel_meth(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
             int meth_index);

static char*
S_parent_offset(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
                const char *meth_type, int meth_index);

static char*
S_overridden_meth(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
                  int meth_index);

static char*
S_inherited_meth(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
                 int meth_index);

static const CFCMeta CFCBINDSPECS_META = {
    "Clownfish::CFC::Binding::Core::Specs",
    sizeof(CFCBindSpecs),
    (CFCBase_destroy_t)CFCBindSpecs_destroy
};

CFCBindSpecs*
CFCBindSpecs_new() {
    CFCBindSpecs *self = (CFCBindSpecs*)CFCBase_allocate(&CFCBINDSPECS_META);
    return CFCBindSpecs_init(self);
}

CFCBindSpecs*
CFCBindSpecs_init(CFCBindSpecs *self) {
    self->novel_specs      = CFCUtil_strdup("");
    self->overridden_specs = CFCUtil_strdup("");
    self->inherited_specs  = CFCUtil_strdup("");
    self->class_specs      = CFCUtil_strdup("");
    self->init_code        = CFCUtil_strdup("");

    return self;
}

void
CFCBindSpecs_destroy(CFCBindSpecs *self) {
    FREEMEM(self->novel_specs);
    FREEMEM(self->overridden_specs);
    FREEMEM(self->inherited_specs);
    FREEMEM(self->class_specs);
    FREEMEM(self->init_code);
    CFCBase_destroy((CFCBase*)self);
}

const char*
CFCBindSpecs_get_typedefs() {
    return
        "/* Structs for Class initialization.\n"
        " */\n"
        "\n"
        "typedef struct cfish_NovelMethSpec {\n"
        "    size_t         *offset;\n"
        "    const char     *name;\n"
        "    cfish_method_t  func;\n"
        "    cfish_method_t  callback_func;\n"
        "} cfish_NovelMethSpec;\n"
        "\n"
        "typedef struct cfish_OverriddenMethSpec {\n"
        "    size_t         *offset;\n"
        "    size_t         *parent_offset;\n"
        "    cfish_method_t  func;\n"
        "} cfish_OverriddenMethSpec;\n"
        "\n"
        "typedef struct cfish_InheritedMethSpec {\n"
        "    size_t *offset;\n"
        "    size_t *parent_offset;\n"
        "} cfish_InheritedMethSpec;\n"
        "\n"
        "typedef struct cfish_ClassSpec {\n"
        "    cfish_Class **klass;\n"
        "    cfish_Class **parent;\n"
        "    const char   *name;\n"
        "    size_t        ivars_size;\n"
        "    size_t       *ivars_offset_ptr;\n"
        "    uint32_t      num_novel_meths;\n"
        "    uint32_t      num_overridden_meths;\n"
        "    uint32_t      num_inherited_meths;\n"
        "    const cfish_NovelMethSpec      *novel_meth_specs;\n"
        "    const cfish_OverriddenMethSpec *overridden_meth_specs;\n"
        "    const cfish_InheritedMethSpec  *inherited_meth_specs;\n"
        "} cfish_ClassSpec;\n"
        "\n";
}

void
CFCBindSpecs_add_class(CFCBindSpecs *self, CFCClass *klass) {
    if (CFCClass_inert(klass)) { return; }

    const char *class_name        = CFCClass_get_name(klass);
    const char *class_var         = CFCClass_full_class_var(klass);
    const char *ivars_offset_name = CFCClass_full_ivars_offset(klass);

    char *ivars_size = S_ivars_size(klass);

    char *parent_ptr = NULL;
    CFCClass *parent = CFCClass_get_parent(klass);
    if (!parent) {
        parent_ptr = CFCUtil_strdup("NULL");
    }
    else {
        if (CFCClass_get_parcel(klass) == CFCClass_get_parcel(parent)) {
            parent_ptr
                = CFCUtil_sprintf("&%s", CFCClass_full_class_var(parent));
        }
        else {
            parent_ptr = CFCUtil_strdup("NULL");

            const char *class_name = CFCClass_get_name(klass);
            const char *parent_var = CFCClass_full_class_var(parent);
            const char *pattern =
                "    /* %s */\n"
                "    class_specs[%d].parent = &%s;\n";
            char *init_code = CFCUtil_sprintf(pattern, class_name,
                                              self->num_specs, parent_var);
            self->init_code = CFCUtil_cat(self->init_code, init_code, NULL);
            FREEMEM(init_code);
        }
    }

    char *novel_specs      = CFCUtil_strdup("");
    char *overridden_specs = CFCUtil_strdup("");
    char *inherited_specs  = CFCUtil_strdup("");
    int num_new_novel      = 0;
    int num_new_overridden = 0;
    int num_new_inherited  = 0;
    CFCMethod **methods = CFCClass_methods(klass);

    for (int meth_num = 0; methods[meth_num] != NULL; meth_num++) {
        CFCMethod *method = methods[meth_num];

        if (CFCMethod_is_fresh(method, klass)) {
            if (CFCMethod_novel(method)) {
                const char *sep = num_new_novel == 0 ? "" : ",\n";
                char *def = S_novel_meth(self, method, klass, num_new_novel);
                novel_specs = CFCUtil_cat(novel_specs, sep, def, NULL);
                FREEMEM(def);
                ++num_new_novel;
            }
            else {
                const char *sep = num_new_overridden == 0 ? "" : ",\n";
                char *def = S_overridden_meth(self, method, klass,
                                              num_new_overridden);
                overridden_specs = CFCUtil_cat(overridden_specs, sep, def,
                                               NULL);
                FREEMEM(def);
                ++num_new_overridden;
            }
        }
        else {
            const char *sep = num_new_inherited == 0 ? "" : ",\n";
            char *def = S_inherited_meth(self, method, klass,
                                         num_new_inherited);
            inherited_specs = CFCUtil_cat(inherited_specs, sep, def, NULL);
            FREEMEM(def);
            ++num_new_inherited;
        }
    }

    char *novel_spec_var;

    if (num_new_novel) {
        novel_spec_var = CFCUtil_sprintf("%s_NOVEL_METHS", class_var);

        const char *pattern =
            "static cfish_NovelMethSpec %s[] = {\n"
            "%s\n"
            "};\n"
            "\n";
        char *spec_def = CFCUtil_sprintf(pattern, novel_spec_var, novel_specs);
        self->novel_specs = CFCUtil_cat(self->novel_specs, spec_def, NULL);
        FREEMEM(spec_def);
    }
    else {
        novel_spec_var = CFCUtil_strdup("NULL");
    }

    char *overridden_spec_var;

    if (num_new_overridden) {
        overridden_spec_var = CFCUtil_sprintf("%s_OVERRIDDEN_METHS",
                                              class_var);

        const char *pattern =
            "static cfish_OverriddenMethSpec %s[] = {\n"
            "%s\n"
            "};\n"
            "\n";
        char *spec_def = CFCUtil_sprintf(pattern, overridden_spec_var,
                                         overridden_specs);
        self->overridden_specs = CFCUtil_cat(self->overridden_specs, spec_def,
                                             NULL);
        FREEMEM(spec_def);
    }
    else {
        overridden_spec_var = CFCUtil_strdup("NULL");
    }

    char *inherited_spec_var;

    if (num_new_inherited) {
        inherited_spec_var = CFCUtil_sprintf("%s_INHERITED_METHS", class_var);

        const char *pattern =
            "static cfish_InheritedMethSpec %s[] = {\n"
            "%s\n"
            "};\n"
            "\n";
        char *spec_def = CFCUtil_sprintf(pattern, inherited_spec_var,
                                         inherited_specs);
        self->inherited_specs = CFCUtil_cat(self->inherited_specs, spec_def,
                                            NULL);
        FREEMEM(spec_def);
    }
    else {
        inherited_spec_var = CFCUtil_strdup("NULL");
    }

    char pattern[] =
        "    {\n"
        "        &%s, /* class */\n"
        "        %s, /* parent */\n"
        "        \"%s\", /* name */\n"
        "        %s, /* ivars_size */\n"
        "        &%s, /* ivars_offset_ptr */\n"
        "        %d, /* num_novel */\n"
        "        %d, /* num_overridden */\n"
        "        %d, /* num_inherited */\n"
        "        %s,\n"
        "        %s,\n"
        "        %s\n"
        "    }";
    char *class_spec
        = CFCUtil_sprintf(pattern, class_var, parent_ptr, class_name,
                          ivars_size, ivars_offset_name,
                          num_new_novel,
                          num_new_overridden,
                          num_new_inherited,
                          novel_spec_var,
                          overridden_spec_var,
                          inherited_spec_var);

    const char *sep = self->num_specs == 0 ? "" : ",\n";
    self->class_specs = CFCUtil_cat(self->class_specs, sep, class_spec, NULL);

    self->num_novel      += num_new_novel;
    self->num_overridden += num_new_overridden;
    self->num_inherited  += num_new_inherited;
    self->num_specs      += 1;

    FREEMEM(class_spec);
    FREEMEM(inherited_spec_var);
    FREEMEM(overridden_spec_var);
    FREEMEM(novel_spec_var);
    FREEMEM(parent_ptr);
    FREEMEM(ivars_size);
}

char*
CFCBindSpecs_defs(CFCBindSpecs *self) {
    if (!self->class_specs[0]) { return CFCUtil_strdup(""); }

    const char *pattern =
        "%s"
        "%s"
        "%s"
        "static cfish_ClassSpec class_specs[] = {\n"
        "%s\n"
        "};\n";
    return CFCUtil_sprintf(pattern, self->novel_specs,
                           self->overridden_specs, self->inherited_specs,
                           self->class_specs);
}

char*
CFCBindSpecs_init_func_def(CFCBindSpecs *self) {
    const char *pattern =
        "static void\n"
        "S_bootstrap_specs() {\n"
        "%s"
        "\n"
        "    cfish_Class_bootstrap(class_specs, %d);\n"
        "}\n";
    return CFCUtil_sprintf(pattern, self->init_code, self->num_specs);
}

static char*
S_ivars_size(CFCClass *klass) {
    CFCParcel *parcel = CFCClass_get_parcel(klass);
    char *ivars_size = NULL;

    if (CFCParcel_is_cfish(parcel)) {
        const char *struct_sym   = CFCClass_full_struct_sym(klass);
        ivars_size = CFCUtil_sprintf("sizeof(%s)", struct_sym);
    }
    else {
        int num_non_package_ivars = CFCClass_num_non_package_ivars(klass);
        int num_ivars             = CFCClass_num_member_vars(klass);

        if (num_non_package_ivars == num_ivars) {
            // No members in this package.
            ivars_size = CFCUtil_strdup("0");
        }
        else {
            const char *ivars_struct = CFCClass_full_ivars_struct(klass);
            ivars_size = CFCUtil_sprintf("sizeof(%s)", ivars_struct);
        }
    }

    return ivars_size;
}

static char*
S_novel_meth(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
             int meth_index) {
    CHY_UNUSED_VAR(self);
    CHY_UNUSED_VAR(meth_index);
    const char *meth_name = CFCMethod_get_name(method);

    char *full_override_sym;
    if (!CFCMethod_final(method)) {
        full_override_sym = CFCMethod_full_override_sym(method, klass);
    }
    else {
        full_override_sym = CFCUtil_strdup("NULL");
    }

    char *imp_func        = CFCMethod_imp_func(method, klass);
    char *full_offset_sym = CFCMethod_full_offset_sym(method, klass);

    char pattern[] =
        "    {\n"
        "        &%s, /* offset */\n"
        "        \"%s\", /* name */\n"
        "        (cfish_method_t)%s, /* func */\n"
        "        (cfish_method_t)%s /* callback_func */\n"
        "    }";
    char *def
        = CFCUtil_sprintf(pattern, full_offset_sym, meth_name, imp_func,
                          full_override_sym);

    FREEMEM(full_offset_sym);
    FREEMEM(imp_func);
    FREEMEM(full_override_sym);
    return def;
}

static char*
S_parent_offset(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
                const char *meth_type, int meth_index) {
    CFCClass *parent = CFCClass_get_parent(klass);

    if (!parent) {
        return CFCUtil_strdup("NULL");
    }

    char *parent_offset = NULL;
    char *parent_offset_sym = CFCMethod_full_offset_sym(method, parent);

    if (CFCClass_get_parcel(parent) == CFCClass_get_parcel(klass)) {
        parent_offset = CFCUtil_sprintf("&%s", parent_offset_sym);
    }
    else {
        parent_offset = CFCUtil_strdup("NULL");

        const char *class_var = CFCClass_full_class_var(klass);
        char pattern[] = "    %s_%s_METHS[%d].parent_offset = &%s;\n";
        char *code = CFCUtil_sprintf(pattern, class_var, meth_type, meth_index,
                                     parent_offset_sym);
        self->init_code = CFCUtil_cat(self->init_code, code, NULL);
        FREEMEM(code);
    }

    FREEMEM(parent_offset_sym);

    return parent_offset;
}

static char*
S_overridden_meth(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
                  int meth_index) {
    char *imp_func        = CFCMethod_imp_func(method, klass);
    char *full_offset_sym = CFCMethod_full_offset_sym(method, klass);
    char *parent_offset   = S_parent_offset(self, method, klass, "OVERRIDDEN",
                                            meth_index);

    char pattern[] =
        "    {\n"
        "        &%s, /* offset */\n"
        "        %s, /* parent_offset */\n"
        "        (cfish_method_t)%s /* func */\n"
        "    }";
    char *def
        = CFCUtil_sprintf(pattern, full_offset_sym, parent_offset, imp_func);

    FREEMEM(parent_offset);
    FREEMEM(full_offset_sym);
    FREEMEM(imp_func);
    return def;
}

static char*
S_inherited_meth(CFCBindSpecs *self, CFCMethod *method, CFCClass *klass,
                 int meth_index) {
    char *full_offset_sym = CFCMethod_full_offset_sym(method, klass);
    char *parent_offset   = S_parent_offset(self, method, klass, "INHERITED",
                                            meth_index);

    char pattern[] =
        "    {\n"
        "        &%s, /* offset */\n"
        "        %s /* parent_offset */\n"
        "    }";
    char *def = CFCUtil_sprintf(pattern, full_offset_sym, parent_offset);

    FREEMEM(full_offset_sym);
    FREEMEM(parent_offset);
    return def;
}


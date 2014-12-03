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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef true
  #define true 1
  #define false 0
#endif

#define CFC_NEED_SYMBOL_STRUCT_DEF
#include "CFCSymbol.h"
#include "CFCClass.h"
#include "CFCFunction.h"
#include "CFCMethod.h"
#include "CFCParcel.h"
#include "CFCDocuComment.h"
#include "CFCUtil.h"
#include "CFCVariable.h"
#include "CFCFileSpec.h"

typedef struct CFCClassRegEntry {
    char *key;
    struct CFCClass *klass;
} CFCClassRegEntry;

static CFCClassRegEntry *registry = NULL;
static size_t registry_size = 0;
static size_t registry_cap  = 0;

// Store a new CFCClass in a registry.
static void
S_register(CFCClass *self);

struct CFCClass {
    CFCSymbol symbol;
    int tree_grown;
    CFCDocuComment *docucomment;
    struct CFCClass *parent;
    struct CFCClass **children;
    size_t num_kids;
    CFCFunction **functions;
    size_t num_functions;
    CFCMethod **fresh_methods;
    size_t num_fresh_meths;
    CFCMethod **methods;
    size_t num_methods;
    CFCVariable **fresh_vars;
    size_t num_fresh_vars;
    CFCVariable **member_vars;
    size_t num_member_vars;
    CFCVariable **inert_vars;
    size_t num_inert_vars;
    CFCFileSpec *file_spec;
    char *parent_class_name;
    int is_final;
    int is_inert;
    char *struct_sym;
    char *full_struct_sym;
    char *ivars_struct;
    char *full_ivars_struct;
    char *ivars_func;
    char *full_ivars_func;
    char *full_ivars_offset;
    char *short_class_var;
    char *full_class_var;
    char *privacy_symbol;
    char *include_h;
};

// Link up parents and kids.
static void
S_establish_ancestry(CFCClass *self);

// Pass down member vars to from parent to children.
static void
S_bequeath_member_vars(CFCClass *self);

// Pass down methods to from parent to children.
static void
S_bequeath_methods(CFCClass *self);

static const CFCMeta CFCCLASS_META = {
    "Clownfish::CFC::Model::Class",
    sizeof(CFCClass),
    (CFCBase_destroy_t)CFCClass_destroy
};

CFCClass*
CFCClass_create(struct CFCParcel *parcel, const char *exposure,
                const char *class_name, const char *nickname,
                const char *micro_sym, CFCDocuComment *docucomment,
                CFCFileSpec *file_spec, const char *parent_class_name,
                int is_final, int is_inert) {
    CFCClass *self = (CFCClass*)CFCBase_allocate(&CFCCLASS_META);
    return CFCClass_do_create(self, parcel, exposure, class_name, nickname,
                              micro_sym, docucomment, file_spec,
                              parent_class_name, is_final, is_inert);
}

CFCClass*
CFCClass_do_create(CFCClass *self, struct CFCParcel *parcel,
                   const char *exposure, const char *class_name,
                   const char *nickname, const char *micro_sym,
                   CFCDocuComment *docucomment, CFCFileSpec *file_spec,
                   const char *parent_class_name, int is_final, int is_inert) {
    CFCUTIL_NULL_CHECK(class_name);
    exposure  = exposure  ? exposure  : "parcel";
    micro_sym = micro_sym ? micro_sym : "class";
    CFCSymbol_init((CFCSymbol*)self, parcel, exposure, class_name, nickname,
                   micro_sym);
    if (!is_inert
        && !parent_class_name
        && strcmp(class_name, "Clownfish::Obj") != 0
       ) {
        parent_class_name = "Clownfish::Obj";
    }
    self->parent     = NULL;
    self->tree_grown = false;
    self->children        = (CFCClass**)CALLOCATE(1, sizeof(CFCClass*));
    self->num_kids        = 0;
    self->functions       = (CFCFunction**)CALLOCATE(1, sizeof(CFCFunction*));
    self->num_functions   = 0;
    self->fresh_methods   = (CFCMethod**)CALLOCATE(1, sizeof(CFCMethod*));
    self->num_fresh_meths = 0;
    self->methods         = NULL;
    self->num_methods     = 0;
    self->fresh_vars      = (CFCVariable**)CALLOCATE(1, sizeof(CFCVariable*));
    self->num_fresh_vars  = 0;
    self->member_vars     = NULL;
    self->num_member_vars = 0;
    self->inert_vars      = (CFCVariable**)CALLOCATE(1, sizeof(CFCVariable*));
    self->num_inert_vars  = 0;
    self->parent_class_name = CFCUtil_strdup(parent_class_name);
    self->docucomment
        = (CFCDocuComment*)CFCBase_incref((CFCBase*)docucomment);
    self->file_spec = (CFCFileSpec*)CFCBase_incref((CFCBase*)file_spec);

    // Cache several derived symbols.
    const char *last_colon = strrchr(class_name, ':');
    self->struct_sym = last_colon
                       ? CFCUtil_strdup(last_colon + 1)
                       : CFCUtil_strdup(class_name);
    const char *prefix = CFCClass_get_prefix(self);
    size_t struct_sym_len = strlen(self->struct_sym);
    self->short_class_var = (char*)MALLOCATE(struct_sym_len + 1);
    size_t i;
    for (i = 0; i < struct_sym_len; i++) {
        self->short_class_var[i] = toupper(self->struct_sym[i]);
    }
    self->short_class_var[struct_sym_len] = '\0';
    self->full_struct_sym   = CFCUtil_sprintf("%s%s", prefix, self->struct_sym);
    self->ivars_struct      = CFCUtil_sprintf("%sIVARS", self->struct_sym);
    self->full_ivars_struct = CFCUtil_sprintf("%sIVARS", self->full_struct_sym);
    self->ivars_func        = CFCUtil_sprintf("%s_IVARS",
                                              CFCClass_get_nickname(self));
    self->full_ivars_func   = CFCUtil_sprintf("%s%s_IVARS", prefix,
                                              CFCClass_get_nickname(self));
    self->full_ivars_offset = CFCUtil_sprintf("%s_OFFSET", self->full_ivars_func);
    size_t full_struct_sym_len = strlen(self->full_struct_sym);
    self->full_class_var = (char*)MALLOCATE(full_struct_sym_len + 1);
    for (i = 0; self->full_struct_sym[i] != '\0'; i++) {
        self->full_class_var[i] = toupper(self->full_struct_sym[i]);
    }
    self->full_class_var[i] = '\0';
    self->privacy_symbol = CFCUtil_sprintf("C_%s", self->full_class_var);

    // Build the relative path to the autogenerated C header file.
    if (file_spec) {
        const char *path_part = CFCFileSpec_get_path_part(self->file_spec);
        self->include_h = CFCUtil_sprintf("%s.h", path_part);
    }
    else {
        self->include_h = CFCUtil_strdup("class.h");
    }

    self->is_final    = !!is_final;
    self->is_inert    = !!is_inert;

    if (!CFCClass_included(self) && CFCParcel_included(parcel)) {
        CFCUtil_die("Class %s from source dir found in parcel %s from"
                    " include dir",
                    class_name, CFCParcel_get_name(parcel));
    }

    // Skip class if it's from an include dir and the parcel was already
    // processed in another source or include dir.
    const char *class_source_dir  = CFCClass_get_source_dir(self);
    const char *parcel_source_dir = CFCParcel_get_source_dir(parcel);
    if (!CFCClass_included(self)
        || !class_source_dir
        || !parcel_source_dir
        || strcmp(class_source_dir, parcel_source_dir) == 0
    ) {
        // Store in registry.
        S_register(self);

        CFCParcel_add_struct_sym(parcel, self->struct_sym);
    }

    return self;
}

static void
S_free_cfcbase_array(CFCBase **array) {
    if (array != NULL) {
        for (size_t i = 0; array[i] != NULL; i++) {
            CFCBase_decref(array[i]);
        }
        FREEMEM(array);
    }
}

void
CFCClass_destroy(CFCClass *self) {
    CFCBase_decref((CFCBase*)self->docucomment);
    CFCBase_decref((CFCBase*)self->parent);
    CFCBase_decref((CFCBase*)self->file_spec);
    S_free_cfcbase_array((CFCBase**)self->children);
    S_free_cfcbase_array((CFCBase**)self->functions);
    S_free_cfcbase_array((CFCBase**)self->fresh_methods);
    S_free_cfcbase_array((CFCBase**)self->methods);
    S_free_cfcbase_array((CFCBase**)self->fresh_vars);
    S_free_cfcbase_array((CFCBase**)self->member_vars);
    S_free_cfcbase_array((CFCBase**)self->inert_vars);
    FREEMEM(self->parent_class_name);
    FREEMEM(self->struct_sym);
    FREEMEM(self->ivars_struct);
    FREEMEM(self->full_ivars_struct);
    FREEMEM(self->ivars_func);
    FREEMEM(self->full_ivars_func);
    FREEMEM(self->full_ivars_offset);
    FREEMEM(self->short_class_var);
    FREEMEM(self->full_struct_sym);
    FREEMEM(self->full_class_var);
    FREEMEM(self->privacy_symbol);
    FREEMEM(self->include_h);
    CFCSymbol_destroy((CFCSymbol*)self);
}

static void
S_register(CFCClass *self) {
    if (registry_size == registry_cap) {
        size_t new_cap = registry_cap + 10;
        registry = (CFCClassRegEntry*)REALLOCATE(
                       registry,
                       (new_cap + 1) * sizeof(CFCClassRegEntry));
        for (size_t i = registry_cap; i <= new_cap; i++) {
            registry[i].key = NULL;
            registry[i].klass = NULL;
        }
        registry_cap = new_cap;
    }

    CFCParcel  *parcel     = CFCClass_get_parcel(self);
    const char *prefix     = CFCParcel_get_prefix(parcel);
    const char *class_name = CFCClass_get_class_name(self);
    const char *nickname   = CFCClass_get_nickname(self);
    const char *key        = self->full_struct_sym;

    for (size_t i = 0; i < registry_size; i++) {
        CFCClass   *other            = registry[i].klass;
        CFCParcel  *other_parcel     = CFCClass_get_parcel(other);
        const char *other_prefix     = CFCParcel_get_prefix(other_parcel);
        const char *other_class_name = CFCClass_get_class_name(other);
        const char *other_nickname   = CFCClass_get_nickname(other);

        if (strcmp(class_name, other_class_name) == 0) {
            CFCUtil_die("Two classes with name %s", class_name);
        }
        if (strcmp(registry[i].key, key) == 0) {
            CFCUtil_die("Class name conflict between %s and %s",
                        class_name, other_class_name);
        }
        if (strcmp(prefix, other_prefix) == 0
            && strcmp(nickname, other_nickname) == 0
           ) {
            CFCUtil_die("Class nickname conflict between %s and %s",
                        class_name, other_class_name);
        }
    }

    registry[registry_size].key   = CFCUtil_strdup(key);
    registry[registry_size].klass = (CFCClass*)CFCBase_incref((CFCBase*)self);
    registry_size++;
}

#define MAX_SINGLETON_LEN 256

CFCClass*
CFCClass_fetch_singleton(CFCParcel *parcel, const char *class_name) {
    CFCUTIL_NULL_CHECK(class_name);

    // Build up the key.
    const char *last_colon = strrchr(class_name, ':');
    const char *struct_sym = last_colon
                             ? last_colon + 1
                             : class_name;
    const char *prefix = parcel ? CFCParcel_get_prefix(parcel) : "";
    size_t prefix_len = strlen(prefix);
    size_t struct_sym_len = strlen(struct_sym);
    if (prefix_len + struct_sym_len > MAX_SINGLETON_LEN) {
        CFCUtil_die("names too long: '%s', '%s'", prefix, struct_sym);
    }
    char key[MAX_SINGLETON_LEN + 1];
    sprintf(key, "%s%s", prefix, struct_sym);
    for (size_t i = 0; i < registry_size; i++) {
        if (strcmp(registry[i].key, key) == 0) {
            return registry[i].klass;
        }
    }
    return NULL;
}

void
CFCClass_clear_registry(void) {
    for (size_t i = 0; i < registry_size; i++) {
        CFCClass *klass = registry[i].klass;
        if (klass->parent) {
            // Break circular ref.
            CFCBase_decref((CFCBase*)klass->parent);
            klass->parent = NULL;
        }
        CFCBase_decref((CFCBase*)klass);
        FREEMEM(registry[i].key);
    }
    FREEMEM(registry);
    registry_size = 0;
    registry_cap  = 0;
    registry      = NULL;
}

void
CFCClass_add_child(CFCClass *self, CFCClass *child) {
    CFCUTIL_NULL_CHECK(child);
    if (self->tree_grown) {
        CFCUtil_die("Can't call add_child after grow_tree");
    }
    if (self->is_inert) {
        CFCUtil_die("Can't inherit from inert class %s",
                    CFCClass_get_class_name(self));
    }
    if (child->is_inert) {
        CFCUtil_die("Inert class %s can't inherit",
                    CFCClass_get_class_name(child));
    }
    self->num_kids++;
    size_t size = (self->num_kids + 1) * sizeof(CFCClass*);
    self->children = (CFCClass**)REALLOCATE(self->children, size);
    self->children[self->num_kids - 1]
        = (CFCClass*)CFCBase_incref((CFCBase*)child);
    self->children[self->num_kids] = NULL;

    // Add parcel dependency.
    CFCParcel *parcel       = CFCClass_get_parcel(self);
    CFCParcel *child_parcel = CFCClass_get_parcel(child);
    if (!CFCParcel_has_prereq(child_parcel, parcel)) {
        CFCUtil_die("Class '%s' inherits from '%s', but parcel '%s' is not a"
                    " prerequisite of '%s'",
                    CFCClass_get_class_name(child),
                    CFCClass_get_class_name(self),
                    CFCParcel_get_name(parcel),
                    CFCParcel_get_name(child_parcel));
    }
    CFCParcel_add_inherited_parcel(child_parcel, parcel);
}

void
CFCClass_add_function(CFCClass *self, CFCFunction *func) {
    CFCUTIL_NULL_CHECK(func);
    if (self->tree_grown) {
        CFCUtil_die("Can't call add_function after grow_tree");
    }
    self->num_functions++;
    size_t size = (self->num_functions + 1) * sizeof(CFCFunction*);
    self->functions = (CFCFunction**)REALLOCATE(self->functions, size);
    self->functions[self->num_functions - 1]
        = (CFCFunction*)CFCBase_incref((CFCBase*)func);
    self->functions[self->num_functions] = NULL;
}

void
CFCClass_add_method(CFCClass *self, CFCMethod *method) {
    CFCUTIL_NULL_CHECK(method);
    if (self->tree_grown) {
        CFCUtil_die("Can't call add_method after grow_tree");
    }
    if (self->is_inert) {
        CFCUtil_die("Can't add_method to an inert class");
    }
    self->num_fresh_meths++;
    size_t size = (self->num_fresh_meths + 1) * sizeof(CFCMethod*);
    self->fresh_methods = (CFCMethod**)REALLOCATE(self->fresh_methods, size);
    self->fresh_methods[self->num_fresh_meths - 1]
        = (CFCMethod*)CFCBase_incref((CFCBase*)method);
    self->fresh_methods[self->num_fresh_meths] = NULL;
}

void
CFCClass_add_member_var(CFCClass *self, CFCVariable *var) {
    CFCUTIL_NULL_CHECK(var);
    if (self->tree_grown) {
        CFCUtil_die("Can't call add_member_var after grow_tree");
    }
    self->num_fresh_vars++;
    size_t size = (self->num_fresh_vars + 1) * sizeof(CFCVariable*);
    self->fresh_vars = (CFCVariable**)REALLOCATE(self->fresh_vars, size);
    self->fresh_vars[self->num_fresh_vars - 1]
        = (CFCVariable*)CFCBase_incref((CFCBase*)var);
    self->fresh_vars[self->num_fresh_vars] = NULL;
}

void
CFCClass_add_inert_var(CFCClass *self, CFCVariable *var) {
    CFCUTIL_NULL_CHECK(var);
    if (self->tree_grown) {
        CFCUtil_die("Can't call add_inert_var after grow_tree");
    }
    self->num_inert_vars++;
    size_t size = (self->num_inert_vars + 1) * sizeof(CFCVariable*);
    self->inert_vars = (CFCVariable**)REALLOCATE(self->inert_vars, size);
    self->inert_vars[self->num_inert_vars - 1]
        = (CFCVariable*)CFCBase_incref((CFCBase*)var);
    self->inert_vars[self->num_inert_vars] = NULL;
}

#define MAX_FUNC_LEN 128

static CFCFunction*
S_find_func(CFCFunction **funcs, const char *sym) {
    if (!sym) {
        return NULL;
    }

    char lcsym[MAX_FUNC_LEN + 1];
    size_t sym_len = strlen(sym);
    if (sym_len > MAX_FUNC_LEN) { CFCUtil_die("sym too long: '%s'", sym); }
    for (size_t i = 0; i <= sym_len; i++) {
        lcsym[i] = tolower(sym[i]);
    }
    for (size_t i = 0; funcs[i] != NULL; i++) {
        CFCFunction *func = funcs[i];
        if (strcmp(lcsym, CFCFunction_micro_sym(func)) == 0) {
            return func;
        }
    }
    return NULL;
}

CFCFunction*
CFCClass_function(CFCClass *self, const char *sym) {
    return S_find_func(self->functions, sym);
}

CFCMethod*
CFCClass_method(CFCClass *self, const char *sym) {
    return (CFCMethod*)S_find_func((CFCFunction**)self->methods, sym);
}

CFCMethod*
CFCClass_fresh_method(CFCClass *self, const char *sym) {
    return (CFCMethod*)S_find_func((CFCFunction**)self->fresh_methods, sym);
}

void
CFCClass_resolve_types(CFCClass *self) {
    for (size_t i = 0; self->functions[i] != NULL; i++) {
        CFCFunction_resolve_types(self->functions[i]);
    }
    for (size_t i = 0; self->fresh_methods[i] != NULL; i++) {
        CFCMethod_resolve_types(self->fresh_methods[i]);
    }
    for (size_t i = 0; self->fresh_vars[i] != NULL; i++) {
        CFCVariable_resolve_type(self->fresh_vars[i]);
    }
    for (size_t i = 0; self->inert_vars[i] != NULL; i++) {
        CFCVariable_resolve_type(self->inert_vars[i]);
    }
}

// Pass down member vars to from parent to children.
static void
S_bequeath_member_vars(CFCClass *self) {
    for (size_t i = 0; self->children[i] != NULL; i++) {
        CFCClass *child = self->children[i];
        size_t num_vars = self->num_member_vars + child->num_fresh_vars;
        size_t size = (num_vars + 1) * sizeof(CFCVariable*);
        child->member_vars
            = (CFCVariable**)REALLOCATE(child->member_vars, size);
        memcpy(child->member_vars, self->member_vars,
               self->num_member_vars * sizeof(CFCVariable*));
        memcpy(child->member_vars + self->num_member_vars,
               child->fresh_vars,
               child->num_fresh_vars * sizeof(CFCVariable*));
        for (size_t j = 0; j < num_vars; j++) {
            CFCBase_incref((CFCBase*)child->member_vars[j]);
        }
        child->num_member_vars = num_vars;
        child->member_vars[num_vars] = NULL;
        S_bequeath_member_vars(child);
    }
}

static void
S_bequeath_methods(CFCClass *self) {
    for (size_t child_num = 0; self->children[child_num] != NULL; child_num++) {
        CFCClass *child = self->children[child_num];

        // Create array of methods, preserving exact order so vtables match up.
        size_t num_methods = 0;
        size_t max_methods = self->num_methods + child->num_fresh_meths;
        CFCMethod **methods = (CFCMethod**)MALLOCATE(
                                  (max_methods + 1) * sizeof(CFCMethod*));

        // Gather methods which child inherits or overrides.
        for (size_t i = 0; i < self->num_methods; i++) {
            CFCMethod *method = self->methods[i];
            const char *macro_sym = CFCMethod_get_macro_sym(method);
            CFCMethod *child_method = CFCClass_fresh_method(child, macro_sym);
            if (child_method) {
                CFCMethod_override(child_method, method);
                methods[num_methods++] = child_method;
            }
            else {
                methods[num_methods++] = method;
            }
        }

        // Append novel child methods to array.  Child methods which were just
        // marked via CFCMethod_override() a moment ago are skipped.
        for (size_t i = 0; i < child->num_fresh_meths; i++) {
            CFCMethod *method = child->fresh_methods[i];
            if (CFCMethod_novel(method)) {
                methods[num_methods++] = method;
            }
        }
        methods[num_methods] = NULL;

        // Manage refcounts and assign new array.  Transform to final methods
        // if child class is a final class.
        if (child->is_final) {
            for (size_t i = 0; i < num_methods; i++) {
                if (CFCMethod_final(methods[i])) {
                    CFCBase_incref((CFCBase*)methods[i]);
                }
                else {
                    methods[i] = CFCMethod_finalize(methods[i]);
                }
            }
        }
        else {
            for (size_t i = 0; i < num_methods; i++) {
                CFCBase_incref((CFCBase*)methods[i]);
            }
        }
        child->methods     = methods;
        child->num_methods = num_methods;

        // Pass it all down to the next generation.
        S_bequeath_methods(child);
        child->tree_grown = true;
    }
}

// Let the children know who their parent class is.
static void
S_establish_ancestry(CFCClass *self) {
    for (size_t i = 0; i < self->num_kids; i++) {
        CFCClass *child = self->children[i];
        // This is a circular reference and thus a memory leak, but we don't
        // care, because we have to have everything in memory at once anyway.
        CFCClass_set_parent(child, self);
        S_establish_ancestry(child);
    }
}

static size_t
S_family_tree_size(CFCClass *self) {
    size_t count = 1; // self
    for (size_t i = 0; i < self->num_kids; i++) {
        count += S_family_tree_size(self->children[i]);
    }
    return count;
}

static CFCBase**
S_copy_cfcbase_array(CFCBase **array, size_t num_elems) {
    CFCBase **copy = (CFCBase**)MALLOCATE((num_elems + 1) * sizeof(CFCBase*));
    for (size_t i = 0; i < num_elems; i++) {
        copy[i] = CFCBase_incref(array[i]);
    }
    copy[num_elems] = NULL;
    return copy;
}

void
CFCClass_grow_tree(CFCClass *self) {
    if (self->tree_grown) {
        CFCUtil_die("Can't call grow_tree more than once");
    }
    S_establish_ancestry(self);

    // Copy fresh variabless for root class.
    self->member_vars
        = (CFCVariable**)S_copy_cfcbase_array((CFCBase**)self->fresh_vars,
                                              self->num_fresh_vars);
    self->num_member_vars = self->num_fresh_vars;

    S_bequeath_member_vars(self);

    // Copy fresh methods for root class.
    self->methods
        = (CFCMethod**)S_copy_cfcbase_array((CFCBase**)self->fresh_methods,
                                            self->num_fresh_meths);
    self->num_methods = self->num_fresh_meths;

    S_bequeath_methods(self);

    self->tree_grown = 1;
}

// Return value is valid only so long as object persists (elements are not
// refcounted).
CFCClass**
CFCClass_tree_to_ladder(CFCClass *self) {
    size_t ladder_len = S_family_tree_size(self);
    CFCClass **ladder = (CFCClass**)MALLOCATE((ladder_len + 1) * sizeof(CFCClass*));
    ladder[ladder_len] = NULL;
    size_t step = 0;
    ladder[step++] = self;
    for (size_t i = 0; i < self->num_kids; i++) {
        CFCClass *child = self->children[i];
        CFCClass **child_ladder = CFCClass_tree_to_ladder(child);
        for (size_t j = 0; child_ladder[j] != NULL; j++) {
            ladder[step++] = child_ladder[j];
        }
        FREEMEM(child_ladder);
    }
    return ladder;
}

CFCMethod**
CFCClass_fresh_methods(CFCClass *self) {
    return self->fresh_methods;
}

CFCVariable**
CFCClass_fresh_member_vars(CFCClass *self) {
    return self->fresh_vars;
}

CFCClass**
CFCClass_children(CFCClass *self) {
    return self->children;
}

CFCFunction**
CFCClass_functions(CFCClass *self) {
    return self->functions;
}

CFCMethod**
CFCClass_methods(CFCClass *self) {
    if (!self->tree_grown) {
        CFCUtil_die("Can't call 'methods' before 'grow_tree'");
    }
    return self->methods;
}

size_t
CFCClass_num_methods(CFCClass *self) {
    if (!self->tree_grown) {
        CFCUtil_die("Can't call 'num_methods' before 'grow_tree'");
    }
    return self->num_methods;
}

CFCVariable**
CFCClass_member_vars(CFCClass *self) {
    if (!self->tree_grown) {
        CFCUtil_die("Can't call 'member_vars' before 'grow_tree'");
    }
    return self->member_vars;
}

size_t
CFCClass_num_member_vars(CFCClass *self) {
    if (!self->tree_grown) {
        CFCUtil_die("Can't call 'num_member_vars' before 'grow_tree'");
    }
    return self->num_member_vars;
}

CFCVariable**
CFCClass_inert_vars(CFCClass *self) {
    return self->inert_vars;
}

const char*
CFCClass_get_nickname(CFCClass *self) {
    return CFCSymbol_get_class_nickname((CFCSymbol*)self);
}

void
CFCClass_set_parent(CFCClass *self, CFCClass *parent) {
    CFCClass *old_parent = self->parent;
    self->parent = (CFCClass*)CFCBase_incref((CFCBase*)parent);
    CFCBase_decref((CFCBase*)old_parent);
}

CFCClass*
CFCClass_get_parent(CFCClass *self) {
    return self->parent;
}

const char*
CFCClass_get_source_dir(CFCClass *self) {
    return self->file_spec
           ? CFCFileSpec_get_source_dir(self->file_spec)
           : NULL;
}

const char*
CFCClass_get_path_part(CFCClass *self) {
    return self->file_spec ? CFCFileSpec_get_path_part(self->file_spec) : NULL;
}

int
CFCClass_included(CFCClass *self) {
    return self->file_spec ? CFCFileSpec_included(self->file_spec) : 0;
}

const char*
CFCClass_get_parent_class_name(CFCClass *self) {
    return self->parent_class_name;
}

int
CFCClass_final(CFCClass *self) {
    return self->is_final;
}

int
CFCClass_inert(CFCClass *self) {
    return self->is_inert;
}

const char*
CFCClass_get_struct_sym(CFCClass *self) {
    return self->struct_sym;
}

const char*
CFCClass_full_struct_sym(CFCClass *self) {
    return self->full_struct_sym;
}

const char*
CFCClass_short_ivars_struct(CFCClass *self) {
    return self->ivars_struct;
}

const char*
CFCClass_full_ivars_struct(CFCClass *self) {
    return self->full_ivars_struct;
}

const char*
CFCClass_short_ivars_func(CFCClass *self) {
    return self->ivars_func;
}

const char*
CFCClass_full_ivars_func(CFCClass *self) {
    return self->full_ivars_func;
}

const char*
CFCClass_full_ivars_offset(CFCClass *self) {
    return self->full_ivars_offset;
}

const char*
CFCClass_short_class_var(CFCClass *self) {
    return self->short_class_var;
}

const char*
CFCClass_full_class_var(CFCClass *self) {
    return self->full_class_var;
}

const char*
CFCClass_privacy_symbol(CFCClass *self) {
    return self->privacy_symbol;
}

const char*
CFCClass_include_h(CFCClass *self) {
    return self->include_h;
}

struct CFCDocuComment*
CFCClass_get_docucomment(CFCClass *self) {
    return self->docucomment;
}

const char*
CFCClass_get_prefix(CFCClass *self) {
    return CFCSymbol_get_prefix((CFCSymbol*)self);
}

const char*
CFCClass_get_Prefix(CFCClass *self) {
    return CFCSymbol_get_Prefix((CFCSymbol*)self);
}

const char*
CFCClass_get_PREFIX(CFCClass *self) {
    return CFCSymbol_get_PREFIX((CFCSymbol*)self);
}

const char*
CFCClass_get_class_name(CFCClass *self) {
    return CFCSymbol_get_class_name((CFCSymbol*)self);
}

CFCParcel*
CFCClass_get_parcel(CFCClass *self) {
    return CFCSymbol_get_parcel((CFCSymbol*)self);
}


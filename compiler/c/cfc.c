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

#include "CFCBase.h"
#include "CFCBindCore.h"
#include "CFCC.h"
#include "CFCClass.h"
#include "CFCLexHeader.h"
#include "CFCHierarchy.h"
#include "CFCParcel.h"
#include "CFCUtil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYS_INCLUDE_DIR "share/clownfish/include"

#if defined(_WIN32) && !defined(__CYGWIN__)
  #define UNIX_FILESYSTEM 0
#else
  #define UNIX_FILESYSTEM 1
#endif

struct CFCArgs {
    char  *dest;
    int    num_source_dirs;
    char **source_dirs;
    int    num_include_dirs;
    char **include_dirs;
    int    num_parcels;
    char **parcels;
    char  *header_filename;
    char  *footer_filename;
};
typedef struct CFCArgs CFCArgs;

static int
S_parse_string_argument(const char *arg, const char *name, char **result) {
    size_t arg_len  = strlen(arg);
    size_t name_len = strlen(name);

    if (arg_len < name_len
        || memcmp(arg, name, name_len) != 0
        || arg[name_len] != '='
       ) {
        return 0;
    }

    if (*result != NULL) {
        fprintf(stderr, "Duplicate %s argument\n", name);
        exit(EXIT_FAILURE);
    }
    *result = CFCUtil_strdup(arg + name_len + 1);

    return 1;
}

static int
S_parse_string_array_argument(const char *arg, const char *name,
                              int *num_results, char ***results) {
    size_t   arg_len  = strlen(arg);
    size_t   name_len = strlen(name);
    int      new_num_results;
    char   **new_results;

    if (arg_len < name_len
        || memcmp(arg, name, name_len) != 0
        || arg[name_len] != '='
       ) {
        return 0;
    }

    new_num_results = *num_results + 1;
    new_results     = (char **)REALLOCATE(*results,
                              (new_num_results + 1) * sizeof(char *));
    new_results[new_num_results-1] = CFCUtil_strdup(arg + name_len + 1);
    new_results[new_num_results]   = NULL;
    *num_results = new_num_results;
    *results     = new_results;

    return 1;
}

/* Parse command line arguments. */
static void
S_parse_arguments(int argc, char **argv, CFCArgs *args) {
    int i;

    memset(args, 0, sizeof(CFCArgs));
    args->source_dirs  = (char**)CALLOCATE(1, sizeof(char*));
    args->include_dirs = (char**)CALLOCATE(1, sizeof(char*));
    args->parcels      = (char**)CALLOCATE(1, sizeof(char*));

    for (i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (S_parse_string_argument(arg, "--dest", &args->dest)) {
            continue;
        }
        if (S_parse_string_argument(arg, "--header", &args->header_filename)) {
            continue;
        }
        if (S_parse_string_argument(arg, "--footer", &args->footer_filename)) {
            continue;
        }
        if (S_parse_string_array_argument(arg, "--source",
                                          &args->num_source_dirs,
                                          &args->source_dirs)
           ) {
            continue;
        }
        if (S_parse_string_array_argument(arg, "--include",
                                          &args->num_include_dirs,
                                          &args->include_dirs)
           ) {
            continue;
        }
        if (S_parse_string_array_argument(arg, "--parcel",
                                          &args->num_parcels,
                                          &args->parcels)
           ) {
            continue;
        }

        fprintf(stderr, "Invalid argument '%s'\n", arg);
        exit(EXIT_FAILURE);
    }

    if (!args->dest) {
        fprintf(stderr, "Mandatory argument --dest missing\n");
        exit(EXIT_FAILURE);
    }
}

static void S_free_arguments(CFCArgs *args) {
    int i;

    if (args->dest)            { FREEMEM(args->dest); }
    if (args->header_filename) { FREEMEM(args->header_filename); }
    if (args->footer_filename) { FREEMEM(args->footer_filename); }

    for (i = 0; args->source_dirs[i]; ++i) {
        FREEMEM(args->source_dirs[i]);
    }
    FREEMEM(args->source_dirs);

    for (i = 0; args->include_dirs[i]; ++i) {
        FREEMEM(args->include_dirs[i]);
    }
    FREEMEM(args->include_dirs);
}

int
main(int argc, char **argv) {
    int           i;
    size_t        file_len;
    CFCArgs       args;
    CFCHierarchy *hierarchy;
    CFCBindCore  *core_binding;
    CFCC         *c_binding;
    char         *header = NULL;
    char         *footer = NULL;
    const char   *include_env;

    S_parse_arguments(argc, argv, &args);

    hierarchy = CFCHierarchy_new(args.dest);

    for (i = 0; args.source_dirs[i]; ++i) {
        CFCHierarchy_add_source_dir(hierarchy, args.source_dirs[i]);
    }
    for (i = 0; args.include_dirs[i]; ++i) {
        CFCHierarchy_add_include_dir(hierarchy, args.include_dirs[i]);
    }

    /* Add include dirs from environment variable CLOWNFISH_INCLUDE. */
    include_env = getenv("CLOWNFISH_INCLUDE");
    if (include_env != NULL) {
        char *include_env_copy = CFCUtil_strdup(include_env);
        const char *include_dir;

        for (include_dir = strtok(include_env_copy, ":");
             include_dir != NULL;
             include_dir = strtok(NULL, ":")
        ) {
            if (include_dir[0] != '\0') {
                CFCHierarchy_add_include_dir(hierarchy, include_dir);
            }
        }

        FREEMEM(include_env_copy);
    }
    else if (UNIX_FILESYSTEM) {
        /*
         * Only add system include dirs if CLOWNFISH_INCLUDE is unset to
         * avoid errors when a parcel is found in multiple locations.
         */
        CFCHierarchy_add_include_dir(hierarchy, "/usr/local/" SYS_INCLUDE_DIR);
        CFCHierarchy_add_include_dir(hierarchy, "/usr/" SYS_INCLUDE_DIR);
    }

    for (i = 0; args.parcels[i]; ++i) {
        CFCHierarchy_add_prereq(hierarchy, args.parcels[i]);
    }

    CFCHierarchy_build(hierarchy);

    if (args.header_filename) {
        header = CFCUtil_slurp_text(args.header_filename, &file_len);
    }
    else {
        header = CFCUtil_strdup("");
    }
    if (args.footer_filename) {
        footer = CFCUtil_slurp_text(args.footer_filename, &file_len);
    }
    else {
        footer = CFCUtil_strdup("");
    }

    core_binding = CFCBindCore_new(hierarchy, header, footer);
    CFCBindCore_write_all_modified(core_binding, 0);

    c_binding = CFCC_new(hierarchy, header, footer);
    CFCC_write_hostdefs(c_binding);
    if (args.num_source_dirs != 0) {
        CFCC_write_callbacks(c_binding);
        CFCC_write_man_pages(c_binding);
    }

    CFCHierarchy_write_log(hierarchy);

    CFCBase_decref((CFCBase*)c_binding);
    CFCBase_decref((CFCBase*)core_binding);
    CFCBase_decref((CFCBase*)hierarchy);
    FREEMEM(header);
    FREEMEM(footer);

    CFCClass_clear_registry();
    CFCParcel_reap_singletons();

    S_free_arguments(&args);

    return EXIT_SUCCESS;
}


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

#define CFC_USE_TEST_MACROS
#include "CFCBase.h"
#include "CFCParcel.h"
#include "CFCSymbol.h"
#include "CFCVersion.h"
#include "CFCTest.h"

#ifndef true
  #define true 1
  #define false 0
#endif

static void
S_run_tests(CFCTest *test);

static void
S_run_prereq_tests(CFCTest *test);

static void
S_run_parcel_tests(CFCTest *test);

const CFCTestBatch CFCTEST_BATCH_PARCEL = {
    "Clownfish::CFC::Model::Parcel",
    23,
    S_run_tests
};

static void
S_run_tests(CFCTest *test) {
    S_run_prereq_tests(test);
    S_run_parcel_tests(test);
}

static void
S_run_prereq_tests(CFCTest *test) {
    {
        CFCVersion *v77_66_55 = CFCVersion_new("v77.66.55");
        CFCPrereq *prereq = CFCPrereq_new("Flour", v77_66_55);
        const char *name = CFCPrereq_get_name(prereq);
        STR_EQ(test, name, "Flour", "prereq get_name");
        CFCVersion *version = CFCPrereq_get_version(prereq);
        INT_EQ(test, CFCVersion_compare_to(version, v77_66_55), 0,
               "prereq get_version");
        CFCBase_decref((CFCBase*)prereq);
        CFCBase_decref((CFCBase*)v77_66_55);
    }

    {
        CFCVersion *v0 = CFCVersion_new("v0");
        CFCPrereq *prereq = CFCPrereq_new("Sugar", NULL);
        CFCVersion *version = CFCPrereq_get_version(prereq);
        INT_EQ(test, CFCVersion_compare_to(version, v0), 0,
               "prereq with default version");
        CFCBase_decref((CFCBase*)prereq);
        CFCBase_decref((CFCBase*)v0);
    }
}

static void
S_run_parcel_tests(CFCTest *test) {
    {
        CFCParcel *parcel = CFCParcel_new("Foo", NULL, NULL, false);
        OK(test, parcel != NULL, "new");
        OK(test, !CFCParcel_included(parcel), "not included");
        CFCBase_decref((CFCBase*)parcel);
    }

    {
        CFCParcel *parcel = CFCParcel_new("Foo", NULL, NULL, true);
        OK(test, CFCParcel_included(parcel), "included");
        CFCBase_decref((CFCBase*)parcel);
    }

    {
        const char *json =
            "        {\n"
            "            \"name\": \"Crustacean\",\n"
            "            \"nickname\": \"Crust\",\n"
            "            \"version\": \"v0.1.0\"\n"
            "        }\n";
        CFCParcel *parcel = CFCParcel_new_from_json(json, false);
        OK(test, parcel != NULL, "new_from_json");
        CFCBase_decref((CFCBase*)parcel);
    }

    {
        const char *path = "t" CHY_DIR_SEP "cfbase" CHY_DIR_SEP "Animal.cfp";
        CFCParcel *parcel = CFCParcel_new_from_file(path, false);
        OK(test, parcel != NULL, "new_from_file");
        CFCBase_decref((CFCBase*)parcel);
    }

    {
        CFCParcel *parcel = CFCParcel_default_parcel();
        CFCSymbol *thing = CFCSymbol_new(parcel, "parcel", NULL, NULL, "sym");
        STR_EQ(test, CFCSymbol_get_prefix(thing), "",
               "get_prefix with no parcel");
        STR_EQ(test, CFCSymbol_get_Prefix(thing), "",
               "get_Prefix with no parcel");
        STR_EQ(test, CFCSymbol_get_PREFIX(thing), "",
               "get_PREFIX with no parcel");
        CFCBase_decref((CFCBase*)thing);
    }

    {
        CFCParcel *parcel = CFCParcel_new("Crustacean", "Crust", NULL, false);
        CFCParcel_register(parcel);
        STR_EQ(test, CFCVersion_get_vstring(CFCParcel_get_version(parcel)),
               "v0", "get_version");

        CFCSymbol *thing = CFCSymbol_new(parcel, "parcel", NULL, NULL, "sym");
        STR_EQ(test, CFCSymbol_get_prefix(thing), "crust_",
               "get_prefix with parcel");
        STR_EQ(test, CFCSymbol_get_Prefix(thing), "Crust_",
               "get_Prefix with parcel");
        STR_EQ(test, CFCSymbol_get_PREFIX(thing), "CRUST_",
               "get_PREFIX with parcel");

        CFCBase_decref((CFCBase*)thing);
        CFCBase_decref((CFCBase*)parcel);
    }

    {
        const char *json =
            "        {\n"
            "            \"name\": \"Crustacean\",\n"
            "            \"version\": \"v0.1.0\",\n"
            "            \"prerequisites\": {\n"
            "                \"Clownfish\": null,\n"
            "                \"Arthropod\": \"v30.104.5\"\n"
            "            }\n"
            "        }\n";
        CFCParcel *parcel = CFCParcel_new_from_json(json, false);

        CFCPrereq **prereqs = CFCParcel_get_prereqs(parcel);
        OK(test, prereqs != NULL, "prereqs");

        CFCPrereq *cfish = prereqs[0];
        OK(test, cfish != NULL, "prereqs[0]");
        const char *cfish_name = CFCPrereq_get_name(cfish);
        STR_EQ(test, cfish_name, "Clownfish", "prereqs[0] name");
        CFCVersion *v0            = CFCVersion_new("v0");
        CFCVersion *cfish_version = CFCPrereq_get_version(cfish);
        INT_EQ(test, CFCVersion_compare_to(cfish_version, v0), 0,
               "prereqs[0] version");

        CFCPrereq *apod = prereqs[1];
        OK(test, apod != NULL, "prereqs[1]");
        const char *apod_name = CFCPrereq_get_name(apod);
        STR_EQ(test, apod_name, "Arthropod", "prereqs[1] name");
        CFCVersion *v30_104_5    = CFCVersion_new("v30.104.5");
        CFCVersion *apod_version = CFCPrereq_get_version(apod);
        INT_EQ(test, CFCVersion_compare_to(apod_version, v30_104_5), 0,
               "prereqs[1] version");

        OK(test, prereqs[2] == NULL, "prereqs[2]");

        CFCBase_decref((CFCBase*)v30_104_5);
        CFCBase_decref((CFCBase*)v0);
        CFCBase_decref((CFCBase*)parcel);
    }

    CFCParcel_reap_singletons();
}


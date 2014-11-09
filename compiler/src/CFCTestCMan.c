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

#include <string.h>

#define CFC_USE_TEST_MACROS
#include "CFCBase.h"
#include "CFCClass.h"
#include "CFCCMan.h"
#include "CFCDocuComment.h"
#include "CFCParcel.h"
#include "CFCTest.h"
#include "CFCUtil.h"

#ifndef true
  #define true 1
  #define false 0
#endif

static void
S_run_tests(CFCTest *test);

const CFCTestBatch CFCTEST_BATCH_C_MAN = {
    "Clownfish::CFC::Binding::C::Man",
    1,
    S_run_tests
};

static void
S_run_tests(CFCTest *test) {
    CFCParcel *parcel = CFCParcel_new("Neato", NULL, NULL, NULL);
    CFCDocuComment *docu = CFCDocuComment_parse(
        "/** Test man page creator.\n"
        " * \n"
        " * # Heading 1\n"
        " * \n"
        " * Paragraph: *emphasized*, **strong**, `code`.\n"
        " * \n"
        " * Paragraph: [link](http://example.com/).\n"
        " * \n"
        " *     Code 1\n"
        " *     Code 2\n"
        " * \n"
        " * * List item 1\n"
        " *   * List item 1.1\n"
        " * \n"
        " *   Paragraph in list\n"
        " * \n"
        " * Paragraph after list\n"
        " */\n"
    );
    CFCClass *klass
        = CFCClass_create(parcel, "public", "Neato::Object", NULL, NULL,
                          docu, NULL, NULL, 0, 0);
    char *man_page = CFCCMan_create_man_page(klass);
    const char *expected_output =
        ".TH Neato::Object 3\n"
        ".SH NAME\n"
        "Neato::Object \\- Test man page creator.\n"
        ".SH DESCRIPTION\n"
        ".SS\n"
        "Heading 1\n"
        "Paragraph: \\fIemphasized\\f[], \\fBstrong\\f[], \\FCcode\\F[]\\&.\n"
        "\n"
        "Paragraph: \n"
        ".UR http://example.com/\n"
        "link\n"
        ".UE\n"
        "\\&.\n"
        ".IP\n"
        ".nf\n"
        ".fam C\n"
        "Code 1\n"
        "Code 2\n"
        ".fam\n"
        ".fi\n"
        ".IP \\(bu\n"
        "List item 1\n"
        ".RS\n"
        ".IP \\(bu\n"
        "List item 1.1\n"
        ".RE\n"
        ".IP\n"
        "Paragraph in list\n"
        ".P\n"
        "Paragraph after list\n";
    STR_EQ(test, man_page, expected_output, "create man page");

    FREEMEM(man_page);
    CFCBase_decref((CFCBase*)klass);
    CFCBase_decref((CFCBase*)docu);
    CFCBase_decref((CFCBase*)parcel);
}


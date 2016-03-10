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

#define C_CFISH_BOOLEAN
#define C_CFISH_CLASS
#define CFISH_USE_SHORT_NAMES
#define TESTCFISH_USE_SHORT_NAMES

#include "charmony.h"

#include <string.h>

#include "Clownfish/Test/TestClass.h"

#include "Clownfish/Boolean.h"
#include "Clownfish/Class.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/Util/Memory.h"

TestClass*
TestClass_new() {
    return (TestClass*)Class_Make_Obj(TESTCLASS);
}

#if DEBUG_CLASS_CONTENTS

#include <stdio.h>

static void
S_memdump(void *vptr, size_t size) {
    unsigned char *ptr = (unsigned char*)vptr;
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", ptr[i]);
    }
    printf("\n");
}

#endif /* DEBUG_CLASS_CONTENTS */

static void
test_bootstrap_idempotence(TestBatchRunner *runner) {
    Class    *bool_class        = BOOLEAN;
    uint32_t  bool_class_size   = BOOLEAN->class_alloc_size;
    uint32_t  bool_ivars_offset = cfish_Bool_IVARS_OFFSET;
    Boolean  *true_singleton    = Bool_true_singleton;

    char *bool_class_contents = (char*)MALLOCATE(bool_class_size);
    memcpy(bool_class_contents, BOOLEAN, bool_class_size);

    // Force another bootstrap run.
    cfish_bootstrap_internal(1);

#if DEBUG_CLASS_CONTENTS
    printf("Before\n");
    S_memdump(bool_class_contents, bool_class_size);
    printf("After\n");
    S_memdump(BOOLEAN, bool_class_size);
#endif

    TEST_TRUE(runner, bool_class == BOOLEAN,
              "Boolean class pointer unchanged");
    TEST_TRUE(runner,
              memcmp(bool_class_contents, BOOLEAN, bool_class_size) == 0,
              "Boolean class unchanged");
    TEST_TRUE(runner, bool_ivars_offset == cfish_Bool_IVARS_OFFSET,
              "Boolean ivars offset unchanged");
    TEST_TRUE(runner, true_singleton == Bool_true_singleton,
              "Boolean singleton unchanged");

    FREEMEM(bool_class_contents);
}

void
TestClass_Run_IMP(TestClass *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 4);
    test_bootstrap_idempotence(runner);
}


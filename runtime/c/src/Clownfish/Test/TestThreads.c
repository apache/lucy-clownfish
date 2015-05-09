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

#define CFISH_USE_SHORT_NAMES
#define TESTCFISH_USE_SHORT_NAMES

#include "charmony.h"

#include "Clownfish/Test/TestThreads.h"

#include "Clownfish/Err.h"
#include "Clownfish/String.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/TestHarness/TestUtils.h"

static void
S_err_thread(void *arg) {
    TestBatchRunner *runner = (TestBatchRunner*)arg;

    TEST_TRUE(runner, Err_get_error() == NULL,
              "global error in thread initialized to null");

    Err_set_error(Err_new(Str_newf("thread")));
}

static void
test_threads(TestBatchRunner *runner) {
    if (!TestUtils_has_threads) {
        SKIP(runner, 2, "no thread support");
        return;
    }

    Err_set_error(Err_new(Str_newf("main")));

    Thread *thread = TestUtils_thread_create(S_err_thread, runner);
    TestUtils_thread_join(thread);

    String *mess = Err_Get_Mess(Err_get_error());
    TEST_TRUE(runner, Str_Equals_Utf8(mess, "main", 4),
              "thread doesn't clobber global error");
}

void
TestThreads_Run_IMP(TestThreads *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 2);
    test_threads(runner);
}


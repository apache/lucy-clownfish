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

/**************************** No thread support ****************************/
#ifdef CFISH_NOTHREADS

static void
test_threads(TestBatchRunner *runner) {
    SKIP(runner, 4, "no thread support");
}

/********************************** Windows ********************************/
#elif defined(CHY_HAS_WINDOWS_H)

#include <windows.h>

static DWORD
S_err_thread(void *arg) {
    TestBatchRunner *runner = (TestBatchRunner*)arg;

    TEST_TRUE(runner, Err_get_error() == NULL,
              "global error in thread initialized to null");

    Err_set_error(Err_new(Str_newf("thread")));

    return 0;
}

static void
test_threads(TestBatchRunner *runner) {
    Err_set_error(Err_new(Str_newf("main")));

    HANDLE thread = CreateThread(NULL, 0, S_err_thread, runner, 0, NULL);
    TEST_TRUE(runner, thread != NULL, "CreateThread succeeds");
    DWORD event = WaitForSingleObject(thread, INFINITE);
    TEST_INT_EQ(runner, event, WAIT_OBJECT_0, "WaitForSingleObject succeeds");
    CloseHandle(thread);

    String *mess = Err_Get_Mess(Err_get_error());
    TEST_TRUE(runner, Str_Equals_Utf8(mess, "main", 4),
              "thread doesn't clobber global error");
}

/******************************** pthreads *********************************/
#elif defined(CHY_HAS_PTHREAD_H)

#include <pthread.h>

static void*
S_err_thread(void *arg) {
    TestBatchRunner *runner = (TestBatchRunner*)arg;

    TEST_TRUE(runner, Err_get_error() == NULL,
              "global error in thread initialized to null");

    Err_set_error(Err_new(Str_newf("thread")));

    return NULL;
}

static void
test_threads(TestBatchRunner *runner) {
    Err_set_error(Err_new(Str_newf("main")));

    int err;
    pthread_t thread;
    err = pthread_create(&thread, NULL, S_err_thread, runner);
    TEST_INT_EQ(runner, err, 0, "pthread_create succeeds");
    err = pthread_join(thread, NULL);
    TEST_INT_EQ(runner, err, 0, "pthread_join succeeds");

    String *mess = Err_Get_Mess(Err_get_error());
    TEST_TRUE(runner, Str_Equals_Utf8(mess, "main", 4),
              "thread doesn't clobber global error");
}

/****************** No support for thread-local storage ********************/
#else

#error "No support for thread-local storage."

#endif

void
TestThreads_Run_IMP(TestThreads *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 4);
    test_threads(runner);
}


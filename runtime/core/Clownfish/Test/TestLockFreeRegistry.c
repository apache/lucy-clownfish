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

#define CFISH_USE_SHORT_NAMES
#define TESTCFISH_USE_SHORT_NAMES

#include "Clownfish/Test/TestLockFreeRegistry.h"

#include "Clownfish/LockFreeRegistry.h"
#include "Clownfish/Test.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/Class.h"

TestLockFreeRegistry*
TestLFReg_new() {
    return (TestLockFreeRegistry*)Class_Make_Obj(TESTLOCKFREEREGISTRY);
}

StupidHashString*
StupidHashString_new(const char *text) {
    StupidHashString *self
        = (StupidHashString*)Class_Make_Obj(STUPIDHASHSTRING);
    return (StupidHashString*)Str_init_from_trusted_utf8((String*)self, text,
                                                         strlen(text));
}

size_t
StupidHashString_Hash_Sum_IMP(StupidHashString *self) {
    UNUSED_VAR(self);
    return 1;
}

static void
test_all(TestBatchRunner *runner) {
    LockFreeRegistry *registry = LFReg_new(10);
    StupidHashString *foo = StupidHashString_new("foo");
    StupidHashString *bar = StupidHashString_new("bar");
    StupidHashString *baz = StupidHashString_new("baz");
    StupidHashString *foo_dupe = StupidHashString_new("foo");

    TEST_TRUE(runner, LFReg_register(registry, (String*)foo, (Obj*)foo),
              "Register() returns true on success");
    TEST_FALSE(runner,
               LFReg_register(registry, (String*)foo_dupe, (Obj*)foo_dupe),
               "Can't Register() keys that test equal");

    TEST_TRUE(runner, LFReg_register(registry, (String*)bar, (Obj*)bar),
              "Register() key with the same Hash_Sum but that isn't Equal");

    TEST_TRUE(runner, LFReg_fetch(registry, (String*)foo_dupe) == (Obj*)foo,
              "Fetch()");
    TEST_TRUE(runner, LFReg_fetch(registry, (String*)bar) == (Obj*)bar,
              "Fetch() again");
    TEST_TRUE(runner, LFReg_fetch(registry, (String*)baz) == NULL,
              "Fetch() non-existent key returns NULL");

    DECREF(foo_dupe);
    DECREF(baz);
    DECREF(bar);
    DECREF(foo);
    LFReg_destroy(registry);
}

void
TestLFReg_Run_IMP(TestLockFreeRegistry *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 6);
    test_all(runner);
}



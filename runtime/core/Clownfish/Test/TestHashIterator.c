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

#include <stdlib.h>
#include <time.h>

#define CFISH_USE_SHORT_NAMES
#define TESTCFISH_USE_SHORT_NAMES

#include "Clownfish/Test/TestHashIterator.h"

#include "Clownfish/Err.h"
#include "Clownfish/String.h"
#include "Clownfish/Hash.h"
#include "Clownfish/HashIterator.h"
#include "Clownfish/Test.h"
#include "Clownfish/VArray.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Class.h"

TestHashIterator*
TestHashIterator_new() {
    return (TestHashIterator*)Class_Make_Obj(TESTHASHITERATOR);
}

static void
test_Next(TestBatchRunner *runner) {
    Hash     *hash     = Hash_new(0); // trigger multiple rebuilds.
    VArray   *expected = VA_new(100);
    VArray   *keys     = VA_new(500);
    VArray   *values   = VA_new(500);

    for (uint32_t i = 0; i < 500; i++) {
        String *str = Str_newf("%u32", i);
        Hash_Store(hash, (Obj*)str, (Obj*)str);
        VA_Push(expected, INCREF(str));
    }

    VA_Sort(expected, NULL, NULL);

    {
        Obj *key;
        Obj *value;
        HashIterator *iter = HashIter_new(hash);
        while (HashIter_Next(iter)) {
            Obj *key = HashIter_Get_Key(iter);
            Obj *value = HashIter_Get_Value(iter);
            VA_Push(keys, INCREF(key));
            VA_Push(values, INCREF(value));
        }
        TEST_TRUE(runner, !HashIter_Next(iter),
                  "Next continues to return false after iteration finishes.");

        DECREF(iter);
    }

    VA_Sort(keys, NULL, NULL);
    VA_Sort(values, NULL, NULL);
    TEST_TRUE(runner, VA_Equals(keys, (Obj*)expected), "Keys from Iter");
    TEST_TRUE(runner, VA_Equals(values, (Obj*)expected), "Values from Iter");

    DECREF(hash);
    DECREF(expected);
    DECREF(keys);
    DECREF(values);
}

static void
test_empty(TestBatchRunner *runner) {
    Hash         *hash = Hash_new(0);
    HashIterator *iter = HashIter_new(hash);

    TEST_TRUE(runner, !HashIter_Next(iter),
              "First call to next false on empty hash iteration");

    TEST_TRUE(runner, !HashIter_Get_Key(iter), "Get_Key returns NULL");
    TEST_TRUE(runner, !HashIter_Get_Value(iter), "Get_Value returns NULL");

    DECREF(hash);
    DECREF(iter);
}

static void
test_Get_Key_and_Get_Value(TestBatchRunner *runner) {
    Hash   *hash = Hash_new(0);
    String *str  = Str_newf("foo");
    Hash_Store(hash, (Obj*)str, (Obj*)str);

    HashIterator *iter = HashIter_new(hash);
    DECREF(hash);

    TEST_TRUE(runner, !HashIter_Get_Key(iter),
              "Get_Key before iteration returns NULL");
    TEST_TRUE(runner, !HashIter_Get_Value(iter),
              "Get_Value before iteration returns NULL");

    HashIter_Next(iter);
    TEST_TRUE(runner, HashIter_Get_Key(iter), "Get_Key during iteration.");
    TEST_TRUE(runner, HashIter_Get_Value(iter), "Get_Value during iteration.");

    HashIter_Next(iter);
    TEST_TRUE(runner, !HashIter_Get_Key(iter), "Get_Key after iteration.");
    TEST_TRUE(runner, !HashIter_Get_Value(iter), "Get_Value after iteration.");

    DECREF(iter);
}

static void
S_invoke_Next(void *context) {
    HashIterator *iter = (HashIterator*)context;
    HashIter_Next(iter);
}

static void
S_invoke_Get_Key(void *context) {
    HashIterator *iter = (HashIterator*)context;
    HashIter_Get_Key(iter);
}

static void
S_invoke_Get_Value(void *context) {
    HashIterator *iter = (HashIterator*)context;
    HashIter_Get_Value(iter);
}

static void
test_illegal_modification(TestBatchRunner *runner) {
    Hash *hash = Hash_new(0);

    for (uint32_t i = 0; i < 3; i++) {
        String *str = Str_newf("%u32", i);
        Hash_Store(hash, (Obj*)str, (Obj*)str);
    }

    HashIterator *iter = HashIter_new(hash);
    HashIter_Next(iter);

    for (uint32_t i = 0; i < 100; i++) {
        String *str = Str_newf("foo %u32", i);
        Hash_Store(hash, (Obj*)str, (Obj*)str);
    }

    Err *next_error = Err_trap(S_invoke_Next, iter);
    TEST_TRUE(runner, next_error != NULL,
              "Next on resized hash throws exception.");
    DECREF(next_error);

    Err *get_key_error = Err_trap(S_invoke_Get_Key, iter);
    TEST_TRUE(runner, get_key_error != NULL,
              "Get_Key on resized hash throws exception.");
    DECREF(get_key_error);

    Err *get_value_error = Err_trap(S_invoke_Get_Value, iter);
    TEST_TRUE(runner, get_value_error != NULL,
              "Get_Value on resized hash throws exception.");
    DECREF(get_value_error);

    DECREF(hash);
    DECREF(iter);
}

static void
test_tombstone(TestBatchRunner *runner) {
    {
        Hash   *hash = Hash_new(0);
        String *str  = Str_newf("foo");
        Hash_Store(hash, (Obj*)str, INCREF(str));
        DECREF(Hash_Delete(hash, (Obj*)str));
        DECREF(str);

	HashIterator *iter = HashIter_new(hash);
	TEST_TRUE(runner, !HashIter_Next(iter), "Next advances past tombstones.");

	DECREF(iter);
    }

    {
        Hash   *hash = Hash_new(0);
        String *str  = Str_newf("foo");
        Hash_Store(hash, (Obj*)str, INCREF(str));

	HashIterator *iter = HashIter_new(hash);
        HashIter_Next(iter);
        DECREF(Hash_Delete(hash, (Obj*)str));

        TEST_TRUE(runner, HashIter_Get_Key(iter) == NULL,
                  "Get_Key doesn't return tombstone");

        DECREF(str);
	DECREF(iter);
    }
}

void
TestHashIterator_Run_IMP(TestHashIterator *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 17);
    srand((unsigned int)time((time_t*)NULL));
    test_Next(runner);
    test_empty(runner);
    test_Get_Key_and_Get_Value(runner);
    test_illegal_modification(runner);
    test_tombstone(runner);
}



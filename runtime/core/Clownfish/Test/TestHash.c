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

#include "Clownfish/Test/TestHash.h"

#include "Clownfish/String.h"
#include "Clownfish/Boolean.h"
#include "Clownfish/Hash.h"
#include "Clownfish/Test.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Vector.h"
#include "Clownfish/Class.h"

TestHash*
TestHash_new() {
    return (TestHash*)Class_Make_Obj(TESTHASH);
}

static void
test_Equals(TestBatchRunner *runner) {
    Hash *hash  = Hash_new(0);
    Hash *other = Hash_new(0);
    String *stuff = SSTR_WRAP_UTF8("stuff", 5);

    TEST_TRUE(runner, Hash_Equals(hash, (Obj*)other),
              "Empty hashes are equal");

    Hash_Store_Utf8(hash, "foo", 3, (Obj*)CFISH_TRUE);
    TEST_FALSE(runner, Hash_Equals(hash, (Obj*)other),
               "Add one pair and Equals returns false");

    Hash_Store_Utf8(other, "foo", 3, (Obj*)CFISH_TRUE);
    TEST_TRUE(runner, Hash_Equals(hash, (Obj*)other),
              "Add a matching pair and Equals returns true");

    Hash_Store_Utf8(other, "foo", 3, INCREF(stuff));
    TEST_FALSE(runner, Hash_Equals(hash, (Obj*)other),
               "Non-matching value spoils Equals");

    DECREF(hash);
    DECREF(other);
}

static void
test_Store_and_Fetch(TestBatchRunner *runner) {
    Hash          *hash         = Hash_new(100);
    Hash          *dupe         = Hash_new(100);
    const size_t   starting_cap = Hash_Get_Capacity(hash);
    Vector        *expected     = Vec_new(100);
    Vector        *got          = Vec_new(100);
    String        *twenty       = SSTR_WRAP_UTF8("20", 2);
    String        *forty        = SSTR_WRAP_UTF8("40", 2);
    String        *foo          = SSTR_WRAP_UTF8("foo", 3);

    for (int32_t i = 0; i < 100; i++) {
        String *str = Str_newf("%i32", i);
        Hash_Store(hash, str, (Obj*)str);
        Hash_Store(dupe, str, INCREF(str));
        Vec_Push(expected, INCREF(str));
    }
    TEST_TRUE(runner, Hash_Equals(hash, (Obj*)dupe), "Equals");

    TEST_INT_EQ(runner, Hash_Get_Capacity(hash), starting_cap,
                "Initial capacity sufficient (no rebuilds)");

    for (int32_t i = 0; i < 100; i++) {
        String *key  = (String*)Vec_Fetch(expected, i);
        Obj    *elem = Hash_Fetch(hash, key);
        Vec_Push(got, (Obj*)INCREF(elem));
    }

    TEST_TRUE(runner, Vec_Equals(got, (Obj*)expected),
              "basic Store and Fetch");
    TEST_INT_EQ(runner, Hash_Get_Size(hash), 100,
                "size incremented properly by Hash_Store");

    TEST_TRUE(runner, Hash_Fetch(hash, foo) == NULL,
              "Fetch against non-existent key returns NULL");

    Obj *stored_foo = INCREF(foo);
    Hash_Store(hash, forty, stored_foo);
    TEST_TRUE(runner, Str_Equals(foo, Hash_Fetch(hash, forty)),
              "Hash_Store replaces existing value");
    TEST_FALSE(runner, Hash_Equals(hash, (Obj*)dupe),
               "replacement value spoils equals");
    TEST_INT_EQ(runner, Hash_Get_Size(hash), 100,
                "size unaffected after value replaced");

    TEST_TRUE(runner, Hash_Delete(hash, forty) == stored_foo,
              "Delete returns value");
    DECREF(stored_foo);
    TEST_INT_EQ(runner, Hash_Get_Size(hash), 99,
                "size decremented by successful Delete");
    TEST_TRUE(runner, Hash_Delete(hash, forty) == NULL,
              "Delete returns NULL when key not found");
    TEST_INT_EQ(runner, Hash_Get_Size(hash), 99,
                "size not decremented by unsuccessful Delete");
    DECREF(Hash_Delete(dupe, forty));
    TEST_TRUE(runner, Vec_Equals(got, (Obj*)expected), "Equals after Delete");

    Hash_Clear(hash);
    TEST_TRUE(runner, Hash_Fetch(hash, twenty) == NULL, "Clear");
    TEST_TRUE(runner, Hash_Get_Size(hash) == 0, "size is 0 after Clear");

    DECREF(hash);
    DECREF(dupe);
    DECREF(got);
    DECREF(expected);
}

static void
test_Keys_Values(TestBatchRunner *runner) {
    Hash     *hash     = Hash_new(0); // trigger multiple rebuilds.
    Vector   *expected = Vec_new(100);
    Vector   *keys;
    Vector   *values;

    for (uint32_t i = 0; i < 500; i++) {
        String *str = Str_newf("%u32", i);
        Hash_Store(hash, str, (Obj*)str);
        Vec_Push(expected, INCREF(str));
    }

    Vec_Sort(expected);

    keys   = Hash_Keys(hash);
    values = Hash_Values(hash);
    Vec_Sort(keys);
    Vec_Sort(values);
    TEST_TRUE(runner, Vec_Equals(keys, (Obj*)expected), "Keys");
    TEST_TRUE(runner, Vec_Equals(values, (Obj*)expected), "Values");
    Vec_Clear(keys);
    Vec_Clear(values);

    {
        String *forty = SSTR_WRAP_UTF8("40", 2);
        String *nope  = SSTR_WRAP_UTF8("nope", 4);
        String *key = Hash_Find_Key(hash, forty, Str_Hash_Sum(forty));
        TEST_TRUE(runner, Str_Equals(key, (Obj*)forty), "Find_Key");
        key = Hash_Find_Key(hash, nope, Str_Hash_Sum(nope)),
        TEST_TRUE(runner, key == NULL,
                  "Find_Key returns NULL for non-existent key");
    }

    DECREF(hash);
    DECREF(expected);
    DECREF(keys);
    DECREF(values);
}

static void
test_stress(TestBatchRunner *runner) {
    Hash     *hash     = Hash_new(0); // trigger multiple rebuilds.
    Vector   *expected = Vec_new(1000);
    Vector   *keys;
    Vector   *values;

    for (uint32_t i = 0; i < 1000; i++) {
        String *str = TestUtils_random_string(rand() % 1200);
        while (Hash_Fetch(hash, str)) {
            DECREF(str);
            str = TestUtils_random_string(rand() % 1200);
        }
        Hash_Store(hash, str, (Obj*)str);
        Vec_Push(expected, INCREF(str));
    }

    Vec_Sort(expected);

    // Overwrite for good measure.
    for (uint32_t i = 0; i < 1000; i++) {
        String *str = (String*)Vec_Fetch(expected, i);
        Hash_Store(hash, str, INCREF(str));
    }

    keys   = Hash_Keys(hash);
    values = Hash_Values(hash);
    Vec_Sort(keys);
    Vec_Sort(values);
    TEST_TRUE(runner, Vec_Equals(keys, (Obj*)expected), "stress Keys");
    TEST_TRUE(runner, Vec_Equals(values, (Obj*)expected), "stress Values");

    DECREF(keys);
    DECREF(values);
    DECREF(expected);
    DECREF(hash);
}

static void
test_store_skips_tombstone(TestBatchRunner *runner) {
    Hash *hash = Hash_new(0);
    size_t mask = Hash_Get_Capacity(hash) - 1;

    String *one = Str_newf("one");
    size_t slot = Str_Hash_Sum(one) & mask;

    // Find a colliding key.
    String *two = NULL;
    for (int i = 0; i < 100000; i++) {
        two = Str_newf("%i32", i);
        if (slot == (Str_Hash_Sum(two) & mask)) {
            break;
        }
        DECREF(two);
        two = NULL;
    }

    Hash_Store(hash, one, (Obj*)CFISH_TRUE);
    Hash_Store(hash, two, (Obj*)CFISH_TRUE);
    Hash_Delete(hash, one);
    Hash_Store(hash, two, (Obj*)CFISH_TRUE);

    TEST_INT_EQ(runner, Hash_Get_Size(hash), 1, "Store skips tombstone");

    DECREF(one);
    DECREF(two);
    DECREF(hash);
}

void
TestHash_Run_IMP(TestHash *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 26);
    srand((unsigned int)time((time_t*)NULL));
    test_Equals(runner);
    test_Store_and_Fetch(runner);
    test_Keys_Values(runner);
    test_stress(runner);
    test_store_skips_tombstone(runner);
}



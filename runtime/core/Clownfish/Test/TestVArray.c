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
#include <stdlib.h>

#define C_CFISH_VARRAY
#define CFISH_USE_SHORT_NAMES
#define TESTCFISH_USE_SHORT_NAMES

#include "Clownfish/Test/TestVArray.h"

#include "Clownfish/String.h"
#include "Clownfish/Err.h"
#include "Clownfish/Num.h"
#include "Clownfish/Test.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/VArray.h"
#include "Clownfish/Class.h"

TestVArray*
TestVArray_new() {
    return (TestVArray*)Class_Make_Obj(TESTVARRAY);
}

// Return an array of size 10 with 30 garbage pointers behind.
static VArray*
S_array_with_garbage() {
    VArray *array = VA_new(100);

    for (int i = 0; i < 40; i++) {
        VA_Push(array, (Obj*)CFISH_TRUE);
    }

    // Remove elements using different methods.
    VA_Excise(array, 10, 10);
    for (int i = 0; i < 10; i++) { VA_Pop(array); }
    VA_Resize(array, 10);

    return array;
}

static void
test_Equals(TestBatchRunner *runner) {
    VArray *array = VA_new(0);
    VArray *other = VA_new(0);
    StackString *stuff = SSTR_WRAP_UTF8("stuff", 5);

    TEST_TRUE(runner, VA_Equals(array, (Obj*)array),
              "Array equal to self");

    TEST_FALSE(runner, VA_Equals(array, (Obj*)CFISH_TRUE),
               "Array not equal to non-array");

    TEST_TRUE(runner, VA_Equals(array, (Obj*)other),
              "Empty arrays are equal");

    VA_Push(array, (Obj*)CFISH_TRUE);
    TEST_FALSE(runner, VA_Equals(array, (Obj*)other),
               "Add one elem and Equals returns false");

    VA_Push(other, (Obj*)CFISH_TRUE);
    TEST_TRUE(runner, VA_Equals(array, (Obj*)other),
              "Add a matching elem and Equals returns true");

    VA_Store(array, 2, (Obj*)CFISH_TRUE);
    TEST_FALSE(runner, VA_Equals(array, (Obj*)other),
               "Add elem after a NULL and Equals returns false");

    VA_Store(other, 2, (Obj*)CFISH_TRUE);
    TEST_TRUE(runner, VA_Equals(array, (Obj*)other),
              "Empty elems don't spoil Equals");

    VA_Store(other, 2, INCREF(stuff));
    TEST_FALSE(runner, VA_Equals(array, (Obj*)other),
               "Non-matching value spoils Equals");

    VA_Store(other, 2, NULL);
    TEST_FALSE(runner, VA_Equals(array, (Obj*)other),
               "NULL value spoils Equals");
    TEST_FALSE(runner, VA_Equals(other, (Obj*)array),
               "NULL value spoils Equals (reversed)");

    VA_Excise(array, 1, 2);       // removes empty elems
    DECREF(VA_Delete(other, 1));  // leaves NULL in place of deleted elem
    DECREF(VA_Delete(other, 2));
    TEST_FALSE(runner, VA_Equals(array, (Obj*)other),
               "Empty trailing elements spoil Equals");

    DECREF(array);
    DECREF(other);
}

static void
test_Store_Fetch(TestBatchRunner *runner) {
    VArray *array = VA_new(0);
    String *elem;

    TEST_TRUE(runner, VA_Fetch(array, 2) == NULL, "Fetch beyond end");

    VA_Store(array, 2, (Obj*)Str_newf("foo"));
    elem = (String*)CERTIFY(VA_Fetch(array, 2), STRING);
    TEST_INT_EQ(runner, 3, VA_Get_Size(array), "Store updates size");
    TEST_TRUE(runner, Str_Equals_Utf8(elem, "foo", 3), "Store");

    elem = (String*)INCREF(elem);
    TEST_INT_EQ(runner, 2, CFISH_REFCOUNT_NN(elem),
                "start with refcount of 2");
    VA_Store(array, 2, (Obj*)Str_newf("bar"));
    TEST_INT_EQ(runner, 1, CFISH_REFCOUNT_NN(elem),
                "Displacing elem via Store updates refcount");
    DECREF(elem);
    elem = (String*)CERTIFY(VA_Fetch(array, 2), STRING);
    TEST_TRUE(runner, Str_Equals_Utf8(elem, "bar", 3), "Store displacement");

    DECREF(array);

    array = S_array_with_garbage();
    VA_Store(array, 40, (Obj*)CFISH_TRUE);
    bool all_null = true;
    for (int i = 10; i < 40; i++) {
        if (VA_Fetch(array, i) != NULL) { all_null = false; }
    }
    TEST_TRUE(runner, all_null, "Out-of-bounds Store clears excised elements");
    DECREF(array);
}

static void
test_Push_Pop_Insert(TestBatchRunner *runner) {
    VArray *array = VA_new(0);
    String *elem;

    TEST_INT_EQ(runner, VA_Get_Size(array), 0, "size starts at 0");
    TEST_TRUE(runner, VA_Pop(array) == NULL,
              "Pop from empty array returns NULL");

    VA_Push(array, (Obj*)Str_newf("a"));
    VA_Push(array, (Obj*)Str_newf("b"));
    VA_Push(array, (Obj*)Str_newf("c"));

    TEST_INT_EQ(runner, VA_Get_Size(array), 3, "size after Push");
    TEST_TRUE(runner, NULL != CERTIFY(VA_Fetch(array, 2), STRING), "Push");

    elem = (String*)CERTIFY(VA_Pop(array), STRING);
    TEST_TRUE(runner, Str_Equals_Utf8(elem, "c", 1), "Pop");
    TEST_INT_EQ(runner, VA_Get_Size(array), 2, "size after Pop");
    DECREF(elem);

    VA_Insert(array, 0, (Obj*)Str_newf("foo"));
    elem = (String*)CERTIFY(VA_Fetch(array, 0), STRING);
    TEST_TRUE(runner, Str_Equals_Utf8(elem, "foo", 3), "Insert");
    TEST_INT_EQ(runner, VA_Get_Size(array), 3, "size after Insert");

    for (int i = 0; i < 256; ++i) {
        VA_Push(array, (Obj*)Str_newf("flotsam"));
    }
    for (int i = 0; i < 512; ++i) {
        VA_Insert(array, i, (Obj*)Str_newf("jetsam"));
    }
    TEST_INT_EQ(runner, VA_Get_Size(array), 3 + 256 + 512,
                "size after exercising Push and Insert");

    DECREF(array);
}

static void
test_Delete(TestBatchRunner *runner) {
    VArray *wanted = VA_new(5);
    VArray *got    = VA_new(5);
    uint32_t i;

    for (i = 0; i < 5; i++) { VA_Push(got, (Obj*)Str_newf("%u32", i)); }
    VA_Store(wanted, 0, (Obj*)Str_newf("0", i));
    VA_Store(wanted, 1, (Obj*)Str_newf("1", i));
    VA_Store(wanted, 4, (Obj*)Str_newf("4", i));
    DECREF(VA_Delete(got, 2));
    DECREF(VA_Delete(got, 3));
    TEST_TRUE(runner, VA_Equals(wanted, (Obj*)got), "Delete");

    TEST_TRUE(runner, VA_Delete(got, 25000) == NULL,
              "Delete beyond array size returns NULL");

    DECREF(wanted);
    DECREF(got);
}

static void
test_Resize(TestBatchRunner *runner) {
    VArray *array = VA_new(3);
    uint32_t i;

    for (i = 0; i < 2; i++) { VA_Push(array, (Obj*)Str_newf("%u32", i)); }
    TEST_INT_EQ(runner, VA_Get_Capacity(array), 3, "Start with capacity 3");

    VA_Resize(array, 4);
    TEST_INT_EQ(runner, VA_Get_Size(array), 4, "Resize up");
    TEST_INT_EQ(runner, VA_Get_Capacity(array), 4,
                "Resize changes capacity");

    VA_Resize(array, 2);
    TEST_INT_EQ(runner, VA_Get_Size(array), 2, "Resize down");
    TEST_TRUE(runner, VA_Fetch(array, 2) == NULL, "Resize down zaps elem");

    VA_Resize(array, 2);
    TEST_INT_EQ(runner, VA_Get_Size(array), 2, "Resize to same size");

    DECREF(array);

    array = S_array_with_garbage();
    VA_Resize(array, 40);
    bool all_null = true;
    for (int i = 10; i < 40; i++) {
        if (VA_Fetch(array, i) != NULL) { all_null = false; }
    }
    TEST_TRUE(runner, all_null, "Resize clears excised elements");
    DECREF(array);
}

static void
test_Excise(TestBatchRunner *runner) {
    VArray *wanted = VA_new(5);
    VArray *got    = VA_new(5);

    for (uint32_t i = 0; i < 5; i++) {
        VA_Push(wanted, (Obj*)Str_newf("%u32", i));
        VA_Push(got, (Obj*)Str_newf("%u32", i));
    }

    VA_Excise(got, 7, 1);
    TEST_TRUE(runner, VA_Equals(wanted, (Obj*)got),
              "Excise outside of range is no-op");

    VA_Excise(got, 2, 2);
    DECREF(VA_Delete(wanted, 2));
    DECREF(VA_Delete(wanted, 3));
    VA_Store(wanted, 2, VA_Delete(wanted, 4));
    VA_Resize(wanted, 3);
    TEST_TRUE(runner, VA_Equals(wanted, (Obj*)got),
              "Excise multiple elems");

    VA_Excise(got, 2, 2);
    VA_Resize(wanted, 2);
    TEST_TRUE(runner, VA_Equals(wanted, (Obj*)got),
              "Splicing too many elems truncates");

    VA_Excise(got, 0, 1);
    VA_Store(wanted, 0, VA_Delete(wanted, 1));
    VA_Resize(wanted, 1);
    TEST_TRUE(runner, VA_Equals(wanted, (Obj*)got),
              "Excise first elem");

    DECREF(got);
    DECREF(wanted);
}

static void
test_Push_All(TestBatchRunner *runner) {
    VArray *wanted  = VA_new(0);
    VArray *got     = VA_new(0);
    VArray *scratch = VA_new(0);
    VArray *empty   = VA_new(0);
    uint32_t i;

    for (i =  0; i < 40; i++) { VA_Push(wanted, (Obj*)Str_newf("%u32", i)); }
    VA_Push(wanted, NULL);
    for (i =  0; i < 20; i++) { VA_Push(got, (Obj*)Str_newf("%u32", i)); }
    for (i = 20; i < 40; i++) { VA_Push(scratch, (Obj*)Str_newf("%u32", i)); }
    VA_Push(scratch, NULL);

    VA_Push_All(got, scratch);
    TEST_TRUE(runner, VA_Equals(wanted, (Obj*)got), "Push_All");

    VA_Push_All(got, empty);
    TEST_TRUE(runner, VA_Equals(wanted, (Obj*)got),
              "Push_All with empty array");

    DECREF(wanted);
    DECREF(got);
    DECREF(scratch);
    DECREF(empty);
}

static void
test_Slice(TestBatchRunner *runner) {
    VArray *array = VA_new(0);
    for (uint32_t i = 0; i < 10; i++) { VA_Push(array, (Obj*)Str_newf("%u32", i)); }
    {
        VArray *slice = VA_Slice(array, 0, 10);
        TEST_TRUE(runner, VA_Equals(array, (Obj*)slice), "Slice entire array");
        DECREF(slice);
    }
    {
        VArray *slice = VA_Slice(array, 0, 11);
        TEST_TRUE(runner, VA_Equals(array, (Obj*)slice),
            "Exceed length");
        DECREF(slice);
    }
    {
        VArray *wanted = VA_new(0);
        VA_Push(wanted, (Obj*)Str_newf("9"));
        VArray *slice = VA_Slice(array, 9, 11);
        TEST_TRUE(runner, VA_Equals(slice, (Obj*)wanted),
            "Exceed length, start near end");
        DECREF(slice);
        DECREF(wanted);
    }
    {
        VArray *slice = VA_Slice(array, 0, 0);
        TEST_TRUE(runner, VA_Get_Size(slice) == 0, "empty slice");
        DECREF(slice);
    }
    {
        VArray *slice = VA_Slice(array, 20, 1);
        TEST_TRUE(runner, VA_Get_Size(slice) ==  0, "exceed offset");
        DECREF(slice);
    }
    {
        VArray *wanted = VA_new(0);
        VA_Push(wanted, (Obj*)Str_newf("9"));
        VArray *slice = VA_Slice(array, 9, SIZE_MAX - 1);
        TEST_TRUE(runner, VA_Get_Size(slice) == 1, "guard against overflow");
        DECREF(slice);
        DECREF(wanted);
    }
    DECREF(array);
}

static void
test_Clone(TestBatchRunner *runner) {
    VArray *array = VA_new(0);
    VArray *twin;
    uint32_t i;

    for (i = 0; i < 10; i++) {
        VA_Push(array, (Obj*)Int32_new(i));
    }
    VA_Push(array, NULL);
    twin = VA_Clone(array);
    TEST_TRUE(runner, VA_Equals(array, (Obj*)twin), "Clone");
    TEST_TRUE(runner, VA_Fetch(array, 1) == VA_Fetch(twin, 1),
              "Clone doesn't clone elements");

    DECREF(array);
    DECREF(twin);
}

static void
S_overflow_Push(void *context) {
    UNUSED_VAR(context);
    VArray *array = VA_new(0);
    array->cap  = SIZE_MAX;
    array->size = array->cap;
    VA_Push(array, (Obj*)CFISH_TRUE);
}

static void
S_overflow_Insert(void *context) {
    UNUSED_VAR(context);
    VArray *array = VA_new(0);
    array->cap  = SIZE_MAX;
    array->size = array->cap;
    VA_Insert(array, 38911, (Obj*)CFISH_TRUE);
}

static void
S_overflow_Push_All(void *context) {
    UNUSED_VAR(context);
    VArray *array = VA_new(0);
    array->cap  = 1000000000;
    array->size = array->cap;
    VArray *other = VA_new(0);
    other->cap  = SIZE_MAX - array->cap + 1;
    other->size = other->cap;
    VA_Push_All(array, other);
}

static void
S_overflow_Store(void *context) {
    UNUSED_VAR(context);
    VArray *array = VA_new(0);
    VA_Store(array, SIZE_MAX, (Obj*)CFISH_TRUE);
}

static void
S_test_exception(TestBatchRunner *runner, Err_Attempt_t func,
                 const char *test_name) {
    Err *error = Err_trap(func, NULL);
    TEST_TRUE(runner, error != NULL, test_name);
    DECREF(error);
}

static void
test_exceptions(TestBatchRunner *runner) {
    if (getenv("LUCY_VALGRIND")) {
        SKIP(runner, 4, "memory leak");
        return;
    }
    S_test_exception(runner, S_overflow_Push,
                     "Push throws on overflow");
    S_test_exception(runner, S_overflow_Insert,
                     "Insert throws on overflow");
    S_test_exception(runner, S_overflow_Push_All,
                     "Push_All throws on overflow");
    S_test_exception(runner, S_overflow_Store,
                     "Store throws on overflow");
}

static void
test_Sort(TestBatchRunner *runner) {
    VArray *array  = VA_new(8);
    VArray *wanted = VA_new(8);

    VA_Push(array, NULL);
    VA_Push(array, (Obj*)Str_newf("aaab"));
    VA_Push(array, (Obj*)Str_newf("ab"));
    VA_Push(array, NULL);
    VA_Push(array, NULL);
    VA_Push(array, (Obj*)Str_newf("aab"));
    VA_Push(array, (Obj*)Str_newf("b"));

    VA_Push(wanted, (Obj*)Str_newf("aaab"));
    VA_Push(wanted, (Obj*)Str_newf("aab"));
    VA_Push(wanted, (Obj*)Str_newf("ab"));
    VA_Push(wanted, (Obj*)Str_newf("b"));
    VA_Push(wanted, NULL);
    VA_Push(wanted, NULL);
    VA_Push(wanted, NULL);

    VA_Sort(array);
    TEST_TRUE(runner, VA_Equals(array, (Obj*)wanted), "Sort with NULLs");

    DECREF(array);
    DECREF(wanted);
}

static void
test_Grow(TestBatchRunner *runner) {
    VArray   *array = VA_new(500);
    uint32_t  cap;

    cap = VA_Get_Capacity(array);
    TEST_TRUE(runner, cap >= 500, "Array is created with minimum capacity");

    VA_Grow(array, 2000);
    cap = VA_Get_Capacity(array);
    TEST_TRUE(runner, cap >= 2000, "Grow to larger capacity");

    uint32_t old_cap = cap;
    VA_Grow(array, old_cap);
    cap = VA_Get_Capacity(array);
    TEST_TRUE(runner, cap >= old_cap, "Grow to same capacity");

    VA_Grow(array, 1000);
    cap = VA_Get_Capacity(array);
    TEST_TRUE(runner, cap >= 1000, "Grow to smaller capacity");

    DECREF(array);
}

void
TestVArray_Run_IMP(TestVArray *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 59);
    test_Equals(runner);
    test_Store_Fetch(runner);
    test_Push_Pop_Insert(runner);
    test_Delete(runner);
    test_Resize(runner);
    test_Excise(runner);
    test_Push_All(runner);
    test_Slice(runner);
    test_Clone(runner);
    test_exceptions(runner);
    test_Sort(runner);
    test_Grow(runner);
}



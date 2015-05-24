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

#include <math.h>

#include "charmony.h"

#include "Clownfish/Test/TestNum.h"

#include "Clownfish/String.h"
#include "Clownfish/Num.h"
#include "Clownfish/Test.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Class.h"

TestNum*
TestNum_new() {
    return (TestNum*)Class_Make_Obj(TESTNUM);
}

static void
test_To_String(TestBatchRunner *runner) {
    Float64   *f64 = Float64_new(1.33);
    Integer64 *i64 = Int64_new(INT64_MAX);
    String *f64_string = Float64_To_String(f64);
    String *i64_string = Int64_To_String(i64);

    TEST_TRUE(runner, Str_Starts_With_Utf8(f64_string, "1.3", 3),
              "Float64_To_String");
    TEST_TRUE(runner, Str_Equals_Utf8(i64_string, "9223372036854775807", 19),
              "Int64_To_String");

    DECREF(i64_string);
    DECREF(f64_string);
    DECREF(i64);
    DECREF(f64);
}

static void
test_accessors(TestBatchRunner *runner) {
    Float64   *f64 = Float64_new(1.0);
    Integer64 *i64 = Int64_new(1);
    double wanted64 = 1.33;
    double got64;

    Float64_Set_Value(f64, 1.33);
    got64 = Float64_Get_Value(f64);
    TEST_TRUE(runner, *(int64_t*)&got64 == *(int64_t*)&wanted64,
              "F64 Set_Value Get_Value");

    TEST_TRUE(runner, Float64_To_I64(f64) == 1, "Float64_To_I64");

    got64 = Float64_To_F64(f64);
    TEST_TRUE(runner, *(int64_t*)&got64 == *(int64_t*)&wanted64,
              "Float64_To_F64");

    Int64_Set_Value(i64, INT64_MIN);
    TEST_TRUE(runner, Int64_Get_Value(i64) == INT64_MIN,
              "I64 Set_Value Get_Value");

    Int64_Set_Value(i64, -1);
    TEST_TRUE(runner, Int64_To_F64(i64) == -1, "Int64_To_F64");

    DECREF(i64);
    DECREF(f64);
}

static void
S_test_compare_float_int(TestBatchRunner *runner, double f64_val,
                         int64_t i64_val, int32_t result) {
    Float64 *f64;
    Integer64 *i64;

    f64 = Float64_new(f64_val);
    i64 = Int64_new(i64_val);
    TEST_INT_EQ(runner, Float64_Compare_To(f64, (Obj*)i64), result,
                "Float64_Compare_To %f %" PRId64, f64_val, i64_val);
    TEST_INT_EQ(runner, Int64_Compare_To(i64, (Obj*)f64), -result,
                "Int64_Compare_To %" PRId64" %f", i64_val, f64_val);
    TEST_INT_EQ(runner, Float64_Equals(f64, (Obj*)i64), result == 0,
                "Float64_Equals %f %" PRId64, f64_val, i64_val);
    TEST_INT_EQ(runner, Int64_Equals(i64, (Obj*)f64), result == 0,
                "Int64_Equals %" PRId64 " %f", i64_val, f64_val);
    DECREF(f64);
    DECREF(i64);

    if (i64_val == INT64_MIN) { return; }

    f64 = Float64_new(-f64_val);
    i64 = Int64_new(-i64_val);
    TEST_INT_EQ(runner, Float64_Compare_To(f64, (Obj*)i64), -result,
                "Float64_Compare_To %f %" PRId64, -f64_val, -i64_val);
    TEST_INT_EQ(runner, Int64_Compare_To(i64, (Obj*)f64), result,
                "Int64_Compare_To %" PRId64" %f", -i64_val, -f64_val);
    TEST_INT_EQ(runner, Float64_Equals(f64, (Obj*)i64), result == 0,
                "Float64_Equals %f %" PRId64, -f64_val, -i64_val);
    TEST_INT_EQ(runner, Int64_Equals(i64, (Obj*)f64), result == 0,
                "Int64_Equals %" PRId64 " %f", -i64_val, -f64_val);
    DECREF(f64);
    DECREF(i64);
}

static void
test_Equals_and_Compare_To(TestBatchRunner *runner) {
    Float64   *f1 = Float64_new(1.0);
    Float64   *f2 = Float64_new(1.0);
    Integer64 *i64 = Int64_new(INT64_MAX);

    TEST_TRUE(runner, Float64_Compare_To(f1, (Obj*)f2) == 0,
              "F64_Compare_To equal");
    TEST_TRUE(runner, Float64_Equals(f1, (Obj*)f2),
              "F64_Equals equal");

    Float64_Set_Value(f2, 2.0);
    TEST_TRUE(runner, Float64_Compare_To(f1, (Obj*)f2) < 0,
              "F64_Compare_To less than");
    TEST_FALSE(runner, Float64_Equals(f1, (Obj*)f2),
               "F64_Equals less than");

    Float64_Set_Value(f2, 0.0);
    TEST_TRUE(runner, Float64_Compare_To(f1, (Obj*)f2) > 0,
              "F64_Compare_To greater than");
    TEST_FALSE(runner, Float64_Equals(f1, (Obj*)f2),
               "F64_Equals greater than");

    Float64_Set_Value(f1, INT64_MAX * 2.0);
    TEST_TRUE(runner, Float64_Compare_To(f1, (Obj*)i64) > 0,
              "Float64 comparison to Integer64");
    TEST_TRUE(runner, Int64_Compare_To(i64, (Obj*)f1) < 0,
              "Integer64 comparison to Float64");

    Int64_Set_Value(i64, INT64_C(0x6666666666666666));
    Integer64 *i64_copy = Int64_new(INT64_C(0x6666666666666666));
    TEST_TRUE(runner, Int64_Compare_To(i64, (Obj*)i64_copy) == 0,
              "Integer64 comparison to same number");

    DECREF(i64_copy);
    DECREF(i64);
    DECREF(f1);
    DECREF(f2);

    // NOTICE: When running these tests on x86/x64, it's best to compile
    // with -ffloat-store to avoid excess FPU precision which can hide
    // implementation bugs.
    S_test_compare_float_int(runner, pow(2.0, 60.0), INT64_C(1) << 60, 0);
    S_test_compare_float_int(runner, pow(2.0, 60.0), (INT64_C(1) << 60) - 1,
                             1);
    S_test_compare_float_int(runner, pow(2.0, 60.0), (INT64_C(1) << 60) + 1,
                             -1);
    S_test_compare_float_int(runner, pow(2.0, 63.0), INT64_MAX, 1);
    S_test_compare_float_int(runner, -pow(2.0, 63.0), INT64_MIN, 0);
    // -9223372036854777856.0 == nextafter(-pow(2, 63), -INFINITY)
    S_test_compare_float_int(runner, -9223372036854777856.0, INT64_MIN, -1);
}

static void
test_Clone(TestBatchRunner *runner) {
    Float64   *f64 = Float64_new(1.33);
    Integer64 *i64 = Int64_new(INT64_MAX);
    Float64   *f64_dupe = Float64_Clone(f64);
    Integer64 *i64_dupe = Int64_Clone(i64);
    TEST_TRUE(runner, Float64_Equals(f64, (Obj*)f64_dupe),
              "Float64 Clone");
    TEST_TRUE(runner, Int64_Equals(i64, (Obj*)i64_dupe),
              "Integer64 Clone");
    DECREF(i64_dupe);
    DECREF(f64_dupe);
    DECREF(i64);
    DECREF(f64);
}

static void
test_Mimic(TestBatchRunner *runner) {
    Float64   *f64 = Float64_new(1.33);
    Integer64 *i64 = Int64_new(INT64_MAX);
    Float64   *f64_dupe = Float64_new(0.0);
    Integer64 *i64_dupe = Int64_new(0);
    Float64_Mimic(f64_dupe, (Obj*)f64);
    Int64_Mimic(i64_dupe, (Obj*)i64);
    TEST_TRUE(runner, Float64_Equals(f64, (Obj*)f64_dupe),
              "Float64 Mimic");
    TEST_TRUE(runner, Int64_Equals(i64, (Obj*)i64_dupe),
              "Integer64 Mimic");
    DECREF(i64_dupe);
    DECREF(f64_dupe);
    DECREF(i64);
    DECREF(f64);
}

void
TestNum_Run_IMP(TestNum *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 60);
    test_To_String(runner);
    test_accessors(runner);
    test_Equals_and_Compare_To(runner);
    test_Clone(runner);
    test_Mimic(runner);
}



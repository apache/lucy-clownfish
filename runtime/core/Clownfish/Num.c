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

#define C_CFISH_INTEGER64
#define C_CFISH_FLOAT64
#define CFISH_USE_SHORT_NAMES

#include <float.h>

#include "charmony.h"

#include "Clownfish/Num.h"
#include "Clownfish/String.h"
#include "Clownfish/Err.h"
#include "Clownfish/Class.h"

#if FLT_RADIX != 2
  #error Unsupported FLT_RADIX
#endif

#if DBL_MANT_DIG != 53
  #error Unsupported DBL_MANT_DIG
#endif

#define MAX_PRECISE_I64 (INT64_C(1) << DBL_MANT_DIG)
#define MIN_PRECISE_I64 -MAX_PRECISE_I64

// For floating point range checks, it's important to use constants that
// can be exactly represented as doubles. `f64 > INT64_MAX` can produce
// wrong results.
#define POW_2_63 9223372036854775808.0

static int32_t
S_compare_f64(double a, double b);

static int32_t
S_compare_i64(int64_t a, int64_t b);

static int32_t
S_compare_i64_f64(int64_t i64, double f64);

static bool
S_equals_i64_f64(int64_t i64, double f64);

Float64*
Float64_new(double value) {
    Float64 *self = (Float64*)Class_Make_Obj(FLOAT64);
    return Float64_init(self, value);
}

Float64*
Float64_init(Float64 *self, double value) {
    self->value = value;
    return self;
}

bool
Float64_Equals_IMP(Float64 *self, Obj *other) {
    if (Obj_is_a(other, FLOAT64)) {
        Float64 *twin = (Float64*)other;
        return self->value == twin->value;
    }
    else if (Obj_is_a(other, INTEGER64)) {
        Integer64 *twin = (Integer64*)other;
        return S_equals_i64_f64(twin->value, self->value);
    }
    else {
        return false;
    }
}

int32_t
Float64_Compare_To_IMP(Float64 *self, Obj *other) {
    if (Obj_is_a(other, FLOAT64)) {
        Float64 *twin = (Float64*)other;
        return S_compare_f64(self->value, twin->value);
    }
    else if (Obj_is_a(other, INTEGER64)) {
        Integer64 *twin = (Integer64*)other;
        return -S_compare_i64_f64(twin->value, self->value);
    }
    else {
        THROW(ERR, "Can't compare Float64 to %o", Obj_get_class_name(other));
        UNREACHABLE_RETURN(int32_t);
    }
}

double
Float64_Get_Value_IMP(Float64 *self) {
    return self->value;
}

void
Float64_Set_Value_IMP(Float64 *self, double value) {
    self->value = value;
}

double
Float64_To_F64_IMP(Float64 *self) {
    return self->value;
}

int64_t
Float64_To_I64_IMP(Float64 *self) {
    if (self->value < -POW_2_63 || self->value >= POW_2_63) {
        THROW(ERR, "Float64 out of range: %f64", self->value);
    }
    return (int64_t)self->value;
}

bool
Float64_To_Bool_IMP(Float64 *self) {
    return self->value != 0.0;
}

String*
Float64_To_String_IMP(Float64 *self) {
    return Str_newf("%f64", self->value);
}

Float64*
Float64_Clone_IMP(Float64 *self) {
    return Float64_new(self->value);
}

void
Float64_Mimic_IMP(Float64 *self, Obj *other) {
    Float64 *twin = (Float64*)CERTIFY(other, FLOAT64);
    self->value = twin->value;
}

/***************************************************************************/

Integer64*
Int64_new(int64_t value) {
    Integer64 *self = (Integer64*)Class_Make_Obj(INTEGER64);
    return Int64_init(self, value);
}

Integer64*
Int64_init(Integer64 *self, int64_t value) {
    self->value = value;
    return self;
}

bool
Int64_Equals_IMP(Integer64 *self, Obj *other) {
    if (Obj_is_a(other, INTEGER64)) {
        Integer64 *twin = (Integer64*)other;
        return self->value == twin->value;
    }
    else if (Obj_is_a(other, FLOAT64)) {
        Float64 *twin = (Float64*)other;
        return S_equals_i64_f64(self->value, twin->value);
    }
    else {
        return false;
    }
}

int32_t
Int64_Compare_To_IMP(Integer64 *self, Obj *other) {
    if (Obj_is_a(other, INTEGER64)) {
        Integer64 *twin = (Integer64*)other;
        return S_compare_i64(self->value, twin->value);
    }
    else if (Obj_is_a(other, FLOAT64)) {
        Float64 *twin = (Float64*)other;
        return S_compare_i64_f64(self->value, twin->value);
    }
    else {
        THROW(ERR, "Can't compare Int64 to %o", Obj_get_class_name(other));
        UNREACHABLE_RETURN(int32_t);
    }
}

int64_t
Int64_Get_Value_IMP(Integer64 *self) {
    return self->value;
}

void
Int64_Set_Value_IMP(Integer64 *self, int64_t value) {
    self->value = value;
}

double
Int64_To_F64_IMP(Integer64 *self) {
    return (double)self->value;
}

int64_t
Int64_To_I64_IMP(Integer64 *self) {
    return self->value;
}

bool
Int64_To_Bool_IMP(Integer64 *self) {
    return self->value != 0;
}

String*
Int64_To_String_IMP(Integer64 *self) {
    return Str_newf("%i64", self->value);
}

Integer64*
Int64_Clone_IMP(Integer64 *self) {
    return Int64_new(self->value);
}

void
Int64_Mimic_IMP(Integer64 *self, Obj *other) {
    Integer64 *twin = (Integer64*)CERTIFY(other, INTEGER64);
    self->value = twin->value;
}

static int32_t
S_compare_f64(double a, double b) {
    return a == b ? 0 : a < b ? -1 : 1;
}

static int32_t
S_compare_i64(int64_t a, int64_t b) {
    return a == b ? 0 : a < b ? -1 : 1;
}

static int32_t
S_compare_i64_f64(int64_t i64, double f64) {
    int f64_comparison = S_compare_f64((double)i64, f64);

    // If the integer can be represented precisely as a double or the numbers
    // compare as unequal when converted to double, the result is correct.
    if ((i64 >= MIN_PRECISE_I64 && i64 <= MAX_PRECISE_I64)
        || f64_comparison != 0
       ) {
        return f64_comparison;
    }

    // Otherwise, the double is an integer.

    // Corner case. 2^63 can compare equal to an int64_t although it is
    // out of range.
    if (f64 == POW_2_63) { return -1; }

    return S_compare_i64(i64, (int64_t)f64);
}

static bool
S_equals_i64_f64(int64_t i64, double f64) {
    bool equal = ((double)i64 == f64);

    // If the integer can be represented precisely as a double or the numbers
    // compare as unequal when converted to double, the result is correct.
    if ((i64 >= MIN_PRECISE_I64 && i64 <= MAX_PRECISE_I64)
        || !equal
       ) {
        return equal;
    }

    // Otherwise, the double is an integer.

    // Corner case. 2^63 can compare equal to an int64_t although it is
    // out of range.
    if (f64 == POW_2_63) { return false; }

    return i64 == (int64_t)f64;
}


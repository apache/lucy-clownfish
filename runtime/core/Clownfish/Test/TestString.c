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
#include <stdio.h>

#define CFISH_USE_SHORT_NAMES
#define TESTCFISH_USE_SHORT_NAMES

#include "Clownfish/Test/TestString.h"

#include "Clownfish/String.h"
#include "Clownfish/Boolean.h"
#include "Clownfish/ByteBuf.h"
#include "Clownfish/CharBuf.h"
#include "Clownfish/Test.h"
#include "Clownfish/TestHarness/TestBatchRunner.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Class.h"

#define SMILEY "\xE2\x98\xBA"
static char smiley[] = { (char)0xE2, (char)0x98, (char)0xBA, 0 };
static uint32_t smiley_len = 3;
static uint32_t smiley_cp  = 0x263A;

TestString*
TestStr_new() {
    return (TestString*)Class_Make_Obj(TESTSTRING);
}

static String*
S_get_str(const char *string) {
    return Str_new_from_utf8(string, strlen(string));
}

// Surround a smiley with lots of whitespace.
static String*
S_smiley_with_whitespace(int *num_spaces_ptr) {
    int32_t spaces[] = {
        ' ',    '\t',   '\r',   '\n',   0x000B, 0x000C, 0x000D, 0x0085,
        0x00A0, 0x1680, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005,
        0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x2028, 0x2029, 0x202F,
        0x205F, 0x3000
    };
    int num_spaces = sizeof(spaces) / sizeof(uint32_t);

    CharBuf *buf = CB_new(0);
    for (int i = 0; i < num_spaces; i++) { CB_Cat_Char(buf, spaces[i]); }
    CB_Cat_Char(buf, smiley_cp);
    for (int i = 0; i < num_spaces; i++) { CB_Cat_Char(buf, spaces[i]); }

    String *retval = CB_To_String(buf);
    if (num_spaces_ptr) { *num_spaces_ptr = num_spaces; }

    DECREF(buf);
    return retval;
}

static void
test_new(TestBatchRunner *runner) {
    static char chars[] = "A string " SMILEY " with a smile.";

    {
        char *buffer = (char*)MALLOCATE(sizeof(chars));
        strcpy(buffer, chars);
        String *thief = Str_new_steal_utf8(buffer, sizeof(chars) - 1);
        TEST_TRUE(runner, Str_Equals_Utf8(thief, chars, sizeof(chars) - 1),
                  "Str_new_steal_utf8");
        DECREF(thief);
    }

    {
        char *buffer = (char*)MALLOCATE(sizeof(chars));
        strcpy(buffer, chars);
        String *thief
            = Str_new_steal_trusted_utf8(buffer, sizeof(chars) - 1);
        TEST_TRUE(runner, Str_Equals_Utf8(thief, chars, sizeof(chars) - 1),
                  "Str_new_steal_trusted_utf8");
        DECREF(thief);
    }

    {
        String *wrapper = Str_new_wrap_utf8(chars, sizeof(chars) - 1);
        TEST_TRUE(runner, Str_Equals_Utf8(wrapper, chars, sizeof(chars) - 1),
                  "Str_new_wrap_utf8");
        DECREF(wrapper);
    }

    {
        String *wrapper = Str_new_wrap_trusted_utf8(chars, sizeof(chars) - 1);
        TEST_TRUE(runner, Str_Equals_Utf8(wrapper, chars, sizeof(chars) - 1),
                  "Str_new_wrap_trusted_utf8");
        DECREF(wrapper);
    }

    {
        String *smiley_str = Str_new_from_char(smiley_cp);
        TEST_TRUE(runner, Str_Equals_Utf8(smiley_str, smiley, smiley_len),
                  "Str_new_from_char");
        DECREF(smiley_str);
    }
}

static void
test_Cat(TestBatchRunner *runner) {
    String *wanted = Str_newf("a%s", smiley);
    String *source;
    String *got;

    source = S_get_str("");
    got = Str_Cat(source, wanted);
    TEST_TRUE(runner, Str_Equals(wanted, (Obj*)got), "Cat");
    DECREF(got);
    DECREF(source);

    source = S_get_str("a");
    got = Str_Cat_Utf8(source, smiley, smiley_len);
    TEST_TRUE(runner, Str_Equals(wanted, (Obj*)got), "Cat_Utf8");
    DECREF(got);
    DECREF(source);

    source = S_get_str("a");
    got = Str_Cat_Trusted_Utf8(source, smiley, smiley_len);
    TEST_TRUE(runner, Str_Equals(wanted, (Obj*)got), "Cat_Trusted_Utf8");
    DECREF(got);
    DECREF(source);

    DECREF(wanted);
}

static void
test_Clone(TestBatchRunner *runner) {
    String *wanted = S_get_str("foo");
    String *got    = Str_Clone(wanted);
    TEST_TRUE(runner, Str_Equals(wanted, (Obj*)got), "Clone");
    DECREF(got);
    DECREF(wanted);
}

static int64_t
S_find(String *string, String *substring) {
    StringIterator *iter = Str_Find(string, substring);
    if (iter == NULL) { return -1; }
    size_t tick = StrIter_Recede(iter, SIZE_MAX);
    DECREF(iter);
    return (int64_t)tick;
}

static void
test_Contains_and_Find(TestBatchRunner *runner) {
    String *string;
    String *substring = S_get_str("foo");

    string = S_get_str("");
    TEST_FALSE(runner, Str_Contains(string, substring),
               "Not contained in empty string");
    TEST_INT_EQ(runner, S_find(string, substring), -1,
                "Not found in empty string");
    DECREF(string);

    string = S_get_str("foo");
    TEST_TRUE(runner, Str_Contains(string, substring),
              "Contained in complete string");
    TEST_INT_EQ(runner, S_find(string, substring), 0, "Find complete string");
    DECREF(string);

    string = S_get_str("afoo");
    TEST_TRUE(runner, Str_Contains(string, substring),
              "Contained after first");
    TEST_INT_EQ(runner, S_find(string, substring), 1, "Find after first");
    String *prefix = Str_SubString(string, 0, 3);
    TEST_FALSE(runner, Str_Contains(prefix, substring), "Don't overrun");
    DECREF(prefix);
    DECREF(string);

    string = S_get_str("afood");
    TEST_TRUE(runner, Str_Contains(string, substring), "Contained in middle");
    TEST_INT_EQ(runner, S_find(string, substring), 1, "Find in middle");
    DECREF(string);

    DECREF(substring);
}

static void
test_Code_Point_At_and_From(TestBatchRunner *runner) {
    int32_t code_points[] = {
        'a', smiley_cp, smiley_cp, 'b', smiley_cp, 'c'
    };
    uint32_t num_code_points = sizeof(code_points) / sizeof(int32_t);
    String *string = Str_newf("a%s%sb%sc", smiley, smiley, smiley);
    uint32_t i;

    for (i = 0; i < num_code_points; i++) {
        uint32_t from = num_code_points - i;
        TEST_INT_EQ(runner, Str_Code_Point_At(string, i), code_points[i],
                    "Code_Point_At %ld", (long)i);
        TEST_INT_EQ(runner, Str_Code_Point_From(string, from),
                    code_points[i], "Code_Point_From %ld", (long)from);
    }

    TEST_INT_EQ(runner, Str_Code_Point_At(string, num_code_points), STR_OOB,
                "Code_Point_At %ld", (long)num_code_points);
    TEST_INT_EQ(runner, Str_Code_Point_From(string, 0), STR_OOB,
                "Code_Point_From 0");
    TEST_INT_EQ(runner, Str_Code_Point_From(string, num_code_points + 1),
                STR_OOB, "Code_Point_From %ld", (long)(num_code_points + 1));

    DECREF(string);
}

static void
test_SubString(TestBatchRunner *runner) {
    {
        String *string = Str_newf("a%s%sb%sc", smiley, smiley, smiley);
        String *wanted = Str_newf("%sb%s", smiley, smiley);
        String *got = Str_SubString(string, 2, 3);
        TEST_TRUE(runner, Str_Equals(wanted, (Obj*)got), "SubString");
        DECREF(string);
        DECREF(wanted);
        DECREF(got);
    }

    {
        static const char chars[] = "A string.";
        String *wrapper = Str_new_wrap_utf8(chars, sizeof(chars) - 1);
        String *wanted  = Str_newf("string");
        String *got     = Str_SubString(wrapper, 2, 6);
        TEST_TRUE(runner, Str_Equals(got, (Obj*)wanted),
                  "SubString with wrapped buffer");
        DECREF(wrapper);
        DECREF(wanted);
        DECREF(got);
    }
}

static void
test_Trim(TestBatchRunner *runner) {
    String *ws_smiley = S_smiley_with_whitespace(NULL);
    String *ws_foo    = S_get_str("  foo  ");
    String *ws_only   = S_get_str("  \t  \r\n");
    String *trimmed   = S_get_str("a     b");
    String *got;

    got = Str_Trim(ws_smiley);
    TEST_TRUE(runner, Str_Equals_Utf8(got, smiley, smiley_len), "Trim");
    DECREF(got);

    got = Str_Trim_Top(ws_foo);
    TEST_TRUE(runner, Str_Equals_Utf8(got, "foo  ", 5), "Trim_Top");
    DECREF(got);

    got = Str_Trim_Tail(ws_foo);
    TEST_TRUE(runner, Str_Equals_Utf8(got, "  foo", 5), "Trim_Tail");
    DECREF(got);

    got = Str_Trim(ws_only);
    TEST_TRUE(runner, Str_Equals_Utf8(got, "", 0), "Trim with only whitespace");
    DECREF(got);

    got = Str_Trim_Top(ws_only);
    TEST_TRUE(runner, Str_Equals_Utf8(got, "", 0),
              "Trim_Top with only whitespace");
    DECREF(got);

    got = Str_Trim_Tail(ws_only);
    TEST_TRUE(runner, Str_Equals_Utf8(got, "", 0),
              "Trim_Tail with only whitespace");
    DECREF(got);

    got = Str_Trim(trimmed);
    TEST_TRUE(runner, Str_Equals(got, (Obj*)trimmed),
              "Trim doesn't change trimmed string");
    DECREF(got);

    got = Str_Trim_Top(trimmed);
    TEST_TRUE(runner, Str_Equals(got, (Obj*)trimmed),
              "Trim_Top doesn't change trimmed string");
    DECREF(got);

    got = Str_Trim_Tail(trimmed);
    TEST_TRUE(runner, Str_Equals(got, (Obj*)trimmed),
              "Trim_Tail doesn't change trimmed string");
    DECREF(got);

    DECREF(trimmed);
    DECREF(ws_only);
    DECREF(ws_foo);
    DECREF(ws_smiley);
}

static void
test_To_F64(TestBatchRunner *runner) {
    String *string;

    string = S_get_str("1.5");
    double difference = 1.5 - Str_To_F64(string);
    if (difference < 0) { difference = 0 - difference; }
    TEST_TRUE(runner, difference < 0.001, "To_F64");
    DECREF(string);

    string = S_get_str("-1.5");
    difference = 1.5 + Str_To_F64(string);
    if (difference < 0) { difference = 0 - difference; }
    TEST_TRUE(runner, difference < 0.001, "To_F64 negative");
    DECREF(string);

    // TODO: Enable this test when we have real substrings.
    /*string = S_get_str("1.59");
    double value_full = Str_To_F64(string);
    Str_Set_Size(string, 3);
    double value_short = Str_To_F64(string);
    TEST_TRUE(runner, value_short < value_full,
              "TO_F64 doesn't run past end of string");
    DECREF(string);*/
}

static void
test_To_I64(TestBatchRunner *runner) {
    String *string;

    string = S_get_str("10");
    TEST_TRUE(runner, Str_To_I64(string) == 10, "To_I64");
    DECREF(string);

    string = S_get_str("-10");
    TEST_TRUE(runner, Str_To_I64(string) == -10, "To_I64 negative");
    DECREF(string);

    string = S_get_str("10.");
    TEST_TRUE(runner, Str_To_I64(string) == 10, "To_I64 stops at non-digits");
    DECREF(string);

    string = S_get_str("10A");
    TEST_TRUE(runner, Str_To_I64(string) == 10,
              "To_I64 stops at out-of-range digits");
    DECREF(string);

    string = S_get_str("-JJ");
    TEST_TRUE(runner, Str_BaseX_To_I64(string, 20) == -399,
              "BaseX_To_I64 base 20");
    DECREF(string);
}

static void
test_To_String(TestBatchRunner *runner) {
    String *string = Str_newf("Test");
    String *copy   = Str_To_String(string);
    TEST_TRUE(runner, Str_Equals(copy, (Obj*)string), "To_String");
    DECREF(string);
    DECREF(copy);
}

static void
test_To_Utf8(TestBatchRunner *runner) {
    String *string = Str_newf("a%s%sb%sc", smiley, smiley, smiley);
    char *buf = Str_To_Utf8(string);
    TEST_TRUE(runner, strcmp(buf, "a" SMILEY SMILEY "b" SMILEY "c") == 0,
              "To_Utf8");
    FREEMEM(buf);
    DECREF(string);
}

static void
test_To_ByteBuf(TestBatchRunner *runner) {
    String     *string = Str_newf("foo");
    ByteBuf    *bb     = Str_To_ByteBuf(string);
    TEST_TRUE(runner, BB_Equals_Bytes(bb, "foo", 3), "To_ByteBuf");
    DECREF(bb);
    DECREF(string);
}

static void
test_Length(TestBatchRunner *runner) {
    String *string = Str_newf("a%s%sb%sc", smiley, smiley, smiley);
    TEST_INT_EQ(runner, Str_Length(string), 6, "Length");
    DECREF(string);
}

static void
test_Compare_To(TestBatchRunner *runner) {
    String *abc = Str_newf("a%s%sb%sc", smiley, smiley, smiley);
    String *ab  = Str_newf("a%s%sb", smiley, smiley);
    String *ac  = Str_newf("a%s%sc", smiley, smiley);

    TEST_TRUE(runner, Str_Compare_To(abc, (Obj*)abc) == 0,
              "Compare_To abc abc");
    TEST_TRUE(runner, Str_Compare_To(ab, (Obj*)abc) < 0,
              "Compare_To ab abc");
    TEST_TRUE(runner, Str_Compare_To(abc, (Obj*)ab) > 0,
              "Compare_To abc ab");
    TEST_TRUE(runner, Str_Compare_To(ab, (Obj*)ac) < 0,
              "Compare_To ab ac");
    TEST_TRUE(runner, Str_Compare_To(ac, (Obj*)ab) > 0,
              "Compare_To ac ab");

    DECREF(ac);
    DECREF(ab);
    DECREF(abc);
}

static void
test_Starts_Ends_With(TestBatchRunner *runner) {
    String *prefix = S_get_str("pre" SMILEY "fix_");
    String *suffix = S_get_str("_post" SMILEY "fix");
    String *empty  = S_get_str("");

    TEST_TRUE(runner, Str_Starts_With(suffix, suffix),
              "Starts_With self returns true");
    TEST_TRUE(runner, Str_Starts_With(prefix, prefix),
              "Ends_With self returns true");

    TEST_TRUE(runner, Str_Starts_With(suffix, empty),
              "Starts_With empty string returns true");
    TEST_TRUE(runner, Str_Ends_With(prefix, empty),
              "Ends_With empty string returns true");
    TEST_FALSE(runner, Str_Starts_With(empty, suffix),
              "Empty string Starts_With returns false");
    TEST_FALSE(runner, Str_Ends_With(empty, prefix),
              "Empty string Ends_With returns false");

    {
        String *string
            = S_get_str("pre" SMILEY "fix_string_post" SMILEY "fix");
        TEST_TRUE(runner, Str_Starts_With(string, prefix),
                  "Starts_With returns true");
        TEST_TRUE(runner, Str_Ends_With(string, suffix),
                  "Ends_With returns true");
        DECREF(string);
    }

    {
        String *string
            = S_get_str("pre" SMILEY "fix:string:post" SMILEY "fix");
        TEST_FALSE(runner, Str_Starts_With(string, prefix),
                   "Starts_With returns false");
        TEST_FALSE(runner, Str_Ends_With(string, suffix),
                   "Ends_With returns false");
        DECREF(string);
    }

    DECREF(prefix);
    DECREF(suffix);
    DECREF(empty);
}

static void
test_Get_Ptr8(TestBatchRunner *runner) {
    String *string = S_get_str("Banana");

    const char *ptr8 = Str_Get_Ptr8(string);
    TEST_TRUE(runner, strcmp(ptr8, "Banana") == 0, "Get_Ptr8");

    size_t size = Str_Get_Size(string);
    TEST_INT_EQ(runner, size, 6, "Get_Size");

    DECREF(string);
}

static void
test_iterator(TestBatchRunner *runner) {
    static const int32_t code_points[] = {
        0x41,
        0x7F,
        0x80,
        0x7FF,
        0x800,
        0xFFFF,
        0x10000,
        0x10FFFF
    };
    static size_t num_code_points
        = sizeof(code_points) / sizeof(code_points[0]);

    CharBuf *buf = CB_new(0);
    for (size_t i = 0; i < num_code_points; ++i) {
        CB_Cat_Char(buf, code_points[i]);
    }
    String *string = CB_To_String(buf);

    {
        StringIterator *iter = Str_Top(string);

        TEST_TRUE(runner, StrIter_Equals(iter, (Obj*)iter),
                  "StringIterator equal to self");
        TEST_FALSE(runner, StrIter_Equals(iter, (Obj*)CFISH_TRUE),
                   "StringIterator not equal non-iterators");

        DECREF(iter);
    }

    {
        StringIterator *top  = Str_Top(string);
        StringIterator *tail = Str_Tail(string);

        TEST_INT_EQ(runner, StrIter_Compare_To(top, (Obj*)tail), -1,
                    "Compare_To top < tail");
        TEST_INT_EQ(runner, StrIter_Compare_To(tail, (Obj*)top), 1,
                    "Compare_To tail > top");
        TEST_INT_EQ(runner, StrIter_Compare_To(top, (Obj*)top), 0,
                    "Compare_To top == top");

        StringIterator *clone = StrIter_Clone(top);
        TEST_TRUE(runner, StrIter_Equals(clone, (Obj*)top), "Clone");

        StrIter_Assign(clone, tail);
        TEST_TRUE(runner, StrIter_Equals(clone, (Obj*)tail), "Assign");

        String *other = Str_newf("Other string");
        StringIterator *other_iter = Str_Top(other);
        TEST_FALSE(runner, StrIter_Equals(other_iter, (Obj*)tail),
                   "Equals returns false for different strings");
        StrIter_Assign(clone, other_iter);
        TEST_TRUE(runner, StrIter_Equals(clone, (Obj*)other_iter),
                  "Assign iterator with different string");

        DECREF(other);
        DECREF(other_iter);
        DECREF(clone);
        DECREF(top);
        DECREF(tail);
    }

    {
        StringIterator *iter = Str_Top(string);

        for (size_t i = 0; i < num_code_points; ++i) {
            TEST_TRUE(runner, StrIter_Has_Next(iter), "Has_Next %d", i);
            int32_t code_point = StrIter_Next(iter);
            TEST_INT_EQ(runner, code_point, code_points[i], "Next %d", i);
        }

        TEST_TRUE(runner, !StrIter_Has_Next(iter),
                  "Has_Next at end of string");
        TEST_INT_EQ(runner, StrIter_Next(iter), STR_OOB,
                    "Next at end of string");

        StringIterator *tail = Str_Tail(string);
        TEST_TRUE(runner, StrIter_Equals(iter, (Obj*)tail), "Equals tail");

        DECREF(tail);
        DECREF(iter);
    }

    {
        StringIterator *iter = Str_Tail(string);

        for (size_t i = num_code_points; i--;) {
            TEST_TRUE(runner, StrIter_Has_Prev(iter), "Has_Prev %d", i);
            int32_t code_point = StrIter_Prev(iter);
            TEST_INT_EQ(runner, code_point, code_points[i], "Prev %d", i);
        }

        TEST_TRUE(runner, !StrIter_Has_Prev(iter),
                  "Has_Prev at end of string");
        TEST_INT_EQ(runner, StrIter_Prev(iter), STR_OOB,
                    "Prev at start of string");

        StringIterator *top = Str_Top(string);
        TEST_TRUE(runner, StrIter_Equals(iter, (Obj*)top), "Equals top");

        DECREF(top);
        DECREF(iter);
    }

    {
        StringIterator *iter = Str_Top(string);

        StrIter_Next(iter);
        TEST_INT_EQ(runner, StrIter_Advance(iter, 2), 2,
                    "Advance returns number of code points");
        TEST_INT_EQ(runner, StrIter_Next(iter), code_points[3],
                    "Advance works");
        TEST_INT_EQ(runner,
                    StrIter_Advance(iter, 1000000), num_code_points - 4,
                    "Advance past end of string");

        StrIter_Prev(iter);
        TEST_INT_EQ(runner, StrIter_Recede(iter, 2), 2,
                    "Recede returns number of code points");
        TEST_INT_EQ(runner, StrIter_Prev(iter), code_points[num_code_points-4],
                    "Recede works");
        TEST_INT_EQ(runner, StrIter_Recede(iter, 1000000), num_code_points - 4,
                    "Recede past start of string");

        DECREF(iter);
    }

    DECREF(string);
    DECREF(buf);
}

static void
test_iterator_whitespace(TestBatchRunner *runner) {
    int num_spaces;
    String *ws_smiley = S_smiley_with_whitespace(&num_spaces);

    {
        StringIterator *iter = Str_Top(ws_smiley);
        TEST_INT_EQ(runner, StrIter_Skip_Whitespace(iter), num_spaces,
                    "Skip_Whitespace");
        TEST_INT_EQ(runner, StrIter_Skip_Whitespace(iter), 0,
                    "Skip_Whitespace without whitespace");
        DECREF(iter);
    }

    {
        StringIterator *iter = Str_Tail(ws_smiley);
        TEST_INT_EQ(runner, StrIter_Skip_Whitespace_Back(iter), num_spaces,
                    "Skip_Whitespace_Back");
        TEST_INT_EQ(runner, StrIter_Skip_Whitespace_Back(iter), 0,
                    "Skip_Whitespace_Back without whitespace");
        DECREF(iter);
    }

    DECREF(ws_smiley);
}

static void
test_iterator_substring(TestBatchRunner *runner) {
    String *string = Str_newf("a%sb%sc%sd", smiley, smiley, smiley);

    StringIterator *start = Str_Top(string);
    StringIterator *end = Str_Tail(string);

    {
        String *substring = StrIter_crop(start, end);
        TEST_TRUE(runner, Str_Equals(substring, (Obj*)string),
                  "StrIter_crop whole string");
        DECREF(substring);
    }

    StrIter_Advance(start, 2);
    StrIter_Recede(end, 2);

    {
        String *substring = StrIter_crop(start, end);
        String *wanted = Str_newf("b%sc", smiley);
        TEST_TRUE(runner, Str_Equals(substring, (Obj*)wanted),
                  "StrIter_crop");

        TEST_TRUE(runner, StrIter_Starts_With(start, wanted),
                  "Starts_With returns true");
        TEST_TRUE(runner, StrIter_Ends_With(end, wanted),
                  "Ends_With returns true");

        DECREF(wanted);
        DECREF(substring);
    }

    {
        String *short_str = Str_newf("b%sx", smiley);
        TEST_FALSE(runner, StrIter_Starts_With(start, short_str),
                   "Starts_With returns false");
        TEST_FALSE(runner, StrIter_Ends_With(start, short_str),
                   "Ends_With returns false");

        String *long_str = Str_newf("b%sxxxxxxxxxxxx%sc", smiley, smiley);
        TEST_FALSE(runner, StrIter_Starts_With(start, long_str),
                   "Starts_With long string returns false");
        TEST_FALSE(runner, StrIter_Ends_With(end, long_str),
                   "Ends_With long string returns false");

        DECREF(short_str);
        DECREF(long_str);
    }

    {
        String *substring = StrIter_crop(end, NULL);
        String *wanted = Str_newf("%sd", smiley);
        TEST_TRUE(runner, Str_Equals(substring, (Obj*)wanted),
                  "StrIter_crop with NULL tail");
        DECREF(wanted);
        DECREF(substring);
    }

    {
        String *substring = StrIter_crop(NULL, start);
        String *wanted = Str_newf("a%s", smiley);
        TEST_TRUE(runner, Str_Equals(substring, (Obj*)wanted),
                  "StrIter_crop with NULL top");
        DECREF(wanted);
        DECREF(substring);
    }

    DECREF(start);
    DECREF(end);
    DECREF(string);
}

void
TestStr_Run_IMP(TestString *self, TestBatchRunner *runner) {
    TestBatchRunner_Plan(runner, (TestBatch*)self, 139);
    test_new(runner);
    test_Cat(runner);
    test_Clone(runner);
    test_Code_Point_At_and_From(runner);
    test_Contains_and_Find(runner);
    test_SubString(runner);
    test_Trim(runner);
    test_To_F64(runner);
    test_To_I64(runner);
    test_To_String(runner);
    test_To_Utf8(runner);
    test_To_ByteBuf(runner);
    test_Length(runner);
    test_Compare_To(runner);
    test_Starts_Ends_With(runner);
    test_Get_Ptr8(runner);
    test_iterator(runner);
    test_iterator_whitespace(runner);
    test_iterator_substring(runner);
}

/*************************** StringCallbackTest ***************************/

bool
StrCbTest_Unchanged_By_Callback_IMP(StringCallbackTest *self, String *str) {
    String *before = Str_Clone(str);
    StrCbTest_Callback(self);
    return Str_Equals(str, (Obj*)before);
}


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

#define C_CFISH_STRINGHELPER
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#define CFISH_USE_SHORT_NAMES

#include "Clownfish/Util/StringHelper.h"
#include "Clownfish/CharBuf.h"
#include "Clownfish/Err.h"
#include "Clownfish/String.h"
#include "Clownfish/Util/Memory.h"

const uint8_t cfish_StrHelp_UTF8_COUNT[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

size_t
StrHelp_overlap(const char *a, const char *b, size_t a_len,  size_t b_len) {
    size_t i;
    const size_t len = a_len <= b_len ? a_len : b_len;

    for (i = 0; i < len; i++) {
        if (*a++ != *b++) { break; }
    }
    return i;
}

static const char base36_chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";

size_t
StrHelp_to_base36(uint64_t num, void *buffer) {
    char  my_buf[StrHelp_MAX_BASE36_BYTES];
    char *buf = my_buf + StrHelp_MAX_BASE36_BYTES - 1;
    char *end = buf;

    // Null terminate.
    *buf = '\0';

    // Convert to base 36 characters.
    do {
        *(--buf) = base36_chars[num % 36];
        num /= 36;
    } while (num > 0);

    size_t size = (size_t)(end - buf);
    memcpy(buffer, buf, size + 1);
    return size;
}

// Return a pointer to the first invalid UTF-8 sequence, or NULL if
// the UTF-8 is valid.
static const uint8_t*
S_find_invalid_utf8(const uint8_t *string, size_t size) {
    const uint8_t *const end = string + size;
    while (string < end) {
        const uint8_t *start = string;
        const uint8_t header_byte = *string++;
        int count = StrHelp_UTF8_COUNT[header_byte] & 0x7;
        switch (count & 0x7) {
            case 1:
                // ASCII
                break;
            case 2:
                if (string == end)              { return start; }
                // Disallow non-shortest-form ASCII.
                if (!(header_byte & 0x1E))      { return start; }
                if ((*string++ & 0xC0) != 0x80) { return start; }
                break;
            case 3:
                if (end - string < 2)           { return start; }
                if (header_byte == 0xED) {
                    // Disallow UTF-16 surrogates.
                    if (*string < 0x80 || *string > 0x9F) {
                        return start;
                    }
                }
                else if (!(header_byte & 0x0F)) {
                    // Disallow non-shortest-form.
                    if (!(*string & 0x20)) {
                        return start;
                    }
                }
                if ((*string++ & 0xC0) != 0x80) { return start; }
                if ((*string++ & 0xC0) != 0x80) { return start; }
                break;
            case 4:
                if (end - string < 3)           { return start; }
                if (!(header_byte & 0x07)) {
                    // Disallow non-shortest-form.
                    if (!(*string & 0x30)) {
                        return start;
                    }
                }
                else if (header_byte == 0xF4) {
                    // Code point larger than 0x10FFFF.
                    if (*string >= 0x90) {
                        return start;
                    }
                }
                if ((*string++ & 0xC0) != 0x80) { return start; }
                if ((*string++ & 0xC0) != 0x80) { return start; }
                if ((*string++ & 0xC0) != 0x80) { return start; }
                break;
            default:
                return start;
        }
    }

    return NULL;
}

bool
StrHelp_utf8_valid(const char *ptr, size_t size) {
    return S_find_invalid_utf8((const uint8_t*)ptr, size) == NULL;
}

void
StrHelp_validate_utf8(const char *ptr, size_t size, const char *file,
                      int line, const char *func) {
    const uint8_t *string  = (const uint8_t*)ptr;
    const uint8_t *invalid = S_find_invalid_utf8(string, size);
    if (invalid == NULL) { return; }

    CharBuf *buf = CB_new(0);
    CB_Cat_Trusted_Utf8(buf, "Invalid UTF-8", 13);

    if (invalid > string) {
        const uint8_t *prefix = invalid;
        size_t num_code_points = 0;

        // Skip up to 20 code points backwards.
        while (prefix > string) {
            prefix -= 1;

            if ((*prefix & 0xC0) != 0x80) {
                num_code_points += 1;
                if (num_code_points >= 20) { break; }
            }
        }

        CB_Cat_Trusted_Utf8(buf, " after '", 8);
        CB_Cat_Trusted_Utf8(buf, (const char*)prefix, invalid - prefix);
        CB_Cat_Trusted_Utf8(buf, "'", 1);
    }

    CB_Cat_Trusted_Utf8(buf, ":", 1);

    // Append offending bytes as hex.
    const uint8_t *end = string + size;
    const uint8_t *max = invalid + 5;
    for (const uint8_t *byte = invalid; byte < end && byte < max; byte++) {
        char hex[4];
        sprintf(hex, " %02X", *byte);
        CB_Cat_Trusted_Utf8(buf, hex, 3);
    }

    String *mess = CB_Yield_String(buf);
    DECREF(buf);

    Err *err = Err_new(mess);
    Err_Add_Frame(err, file, line, func);
    Err_do_throw(err);
}

bool
StrHelp_is_whitespace(int32_t code_point) {
    switch (code_point) {
            // <control-0009>..<control-000D>
        case 0x0009: case 0x000A: case 0x000B: case 0x000C: case 0x000D:
        case 0x0020: // SPACE
        case 0x0085: // <control-0085>
        case 0x00A0: // NO-BREAK SPACE
        case 0x1680: // OGHAM SPACE MARK
            // EN QUAD..HAIR SPACE
        case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004:
        case 0x2005: case 0x2006: case 0x2007: case 0x2008: case 0x2009:
        case 0x200A:
        case 0x2028: // LINE SEPARATOR
        case 0x2029: // PARAGRAPH SEPARATOR
        case 0x202F: // NARROW NO-BREAK SPACE
        case 0x205F: // MEDIUM MATHEMATICAL SPACE
        case 0x3000: // IDEOGRAPHIC SPACE
            return true;

        default:
            return false;
    }
}

uint32_t
StrHelp_encode_utf8_char(int32_t code_point, void *buffer) {
    uint8_t *buf = (uint8_t*)buffer;
    if (code_point <= 0x7F) { // ASCII
        buf[0] = (uint8_t)code_point;
        return 1;
    }
    else if (code_point <= 0x07FF) { // 2 byte range
        buf[0] = (uint8_t)(0xC0 | (code_point >> 6));
        buf[1] = (uint8_t)(0x80 | (code_point & 0x3f));
        return 2;
    }
    else if (code_point <= 0xFFFF) { // 3 byte range
        buf[0] = (uint8_t)(0xE0 | (code_point  >> 12));
        buf[1] = (uint8_t)(0x80 | ((code_point >> 6) & 0x3F));
        buf[2] = (uint8_t)(0x80 | (code_point        & 0x3f));
        return 3;
    }
    else if (code_point <= 0x10FFFF) { // 4 byte range
        buf[0] = (uint8_t)(0xF0 | (code_point  >> 18));
        buf[1] = (uint8_t)(0x80 | ((code_point >> 12) & 0x3F));
        buf[2] = (uint8_t)(0x80 | ((code_point >> 6)  & 0x3F));
        buf[3] = (uint8_t)(0x80 | (code_point         & 0x3f));
        return 4;
    }
    else {
        THROW(ERR, "Illegal Unicode code point: %u32", code_point);
        UNREACHABLE_RETURN(uint32_t);
    }
}

const char*
StrHelp_back_utf8_char(const char *ptr, const char *start) {
    while (--ptr >= start) {
        if ((*ptr & 0xC0) != 0x80) { return ptr; }
    }
    return NULL;
}


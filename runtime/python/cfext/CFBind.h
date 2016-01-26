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

#ifndef H_CFISH_PY_CFBIND
#define H_CFISH_PY_CFBIND 1

#ifdef __cplusplus
extern "C" {
#endif

#include "cfish_platform.h"
#include "Python.h"
#include "Clownfish/Obj.h"

struct cfish_Class;
struct cfish_String;

/** Wrap the current state of Python's sys.exc_info in a Clownfish Err and
  * throw it.
  *
  * One refcount of `mess` will be consumed by this function.
  *
  * TODO: Leave the original exception intact.
  */
void
CFBind_reraise_pyerr(struct cfish_Class *err_klass, struct cfish_String *mess);

/** Null-safe invocation of Obj_To_Host.
  */
static CFISH_INLINE PyObject*
CFBind_cfish_to_py(struct cfish_Obj *obj) {
    if (obj != NULL) {
        return (PyObject*)CFISH_Obj_To_Host(obj);
    }
    else {
        Py_RETURN_NONE;
    }
}

/** Perform the same conversion as `CFBind_cfish_to_py`, but ensure that the
  * result is refcount-neutral, decrementing a refcount from `obj` and passing
  * it along.
  */
static CFISH_INLINE PyObject*
CFBind_cfish_to_py_zeroref(struct cfish_Obj *obj) {
    if (obj != NULL) {
        PyObject *result = (PyObject*)CFISH_Obj_To_Host(obj);
        CFISH_DECREF(obj);
        return result;
    }
    else {
        return Py_None;
    }
}

/* ParseTuple conversion routines for primitive numeric types.
 *
 * If the value of `input` is out of range for the an integer C type, an
 * OverflowError will be raised.
 *
 * If `input` is `None`, the "maybe_convert" variants will leave `ptr`
 * untouched, while the "convert" routines will raise a TypeError.
 */
int
CFBind_convert_char(PyObject *input, char *ptr);
int
CFBind_convert_short(PyObject *input, short *ptr);
int
CFBind_convert_int(PyObject *input, int *ptr);
int
CFBind_convert_long(PyObject *input, long *ptr);
int
CFBind_convert_int8_t(PyObject *input, int8_t *ptr);
int
CFBind_convert_int16_t(PyObject *input, int16_t *ptr);
int
CFBind_convert_int32_t(PyObject *input, int32_t *ptr);
int
CFBind_convert_int64_t(PyObject *input, int64_t *ptr);
int
CFBind_convert_uint8_t(PyObject *input, uint8_t *ptr);
int
CFBind_convert_uint16_t(PyObject *input, uint16_t *ptr);
int
CFBind_convert_uint32_t(PyObject *input, uint32_t *ptr);
int
CFBind_convert_uint64_t(PyObject *input, uint64_t *ptr);
int
CFBind_convert_bool(PyObject *input, bool *ptr);
int
CFBind_convert_size_t(PyObject *input, size_t *ptr);
int
CFBind_convert_float(PyObject *input, float *ptr);
int
CFBind_convert_double(PyObject *input, double *ptr);
int
CFBind_maybe_convert_char(PyObject *input, char *ptr);
int
CFBind_maybe_convert_short(PyObject *input, short *ptr);
int
CFBind_maybe_convert_int(PyObject *input, int *ptr);
int
CFBind_maybe_convert_long(PyObject *input, long *ptr);
int
CFBind_maybe_convert_int8_t(PyObject *input, int8_t *ptr);
int
CFBind_maybe_convert_int16_t(PyObject *input, int16_t *ptr);
int
CFBind_maybe_convert_int32_t(PyObject *input, int32_t *ptr);
int
CFBind_maybe_convert_int64_t(PyObject *input, int64_t *ptr);
int
CFBind_maybe_convert_uint8_t(PyObject *input, uint8_t *ptr);
int
CFBind_maybe_convert_uint16_t(PyObject *input, uint16_t *ptr);
int
CFBind_maybe_convert_uint32_t(PyObject *input, uint32_t *ptr);
int
CFBind_maybe_convert_uint64_t(PyObject *input, uint64_t *ptr);
int
CFBind_maybe_convert_bool(PyObject *input, bool *ptr);
int
CFBind_maybe_convert_size_t(PyObject *input, size_t *ptr);
int
CFBind_maybe_convert_float(PyObject *input, float *ptr);
int
CFBind_maybe_convert_double(PyObject *input, double *ptr);

#ifdef __cplusplus
}
#endif

#endif // H_CFISH_PY_CFBIND


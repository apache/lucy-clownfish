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

#define C_CFISH_OBJ
#define C_CFISH_CLASS
#define C_CFISH_METHOD
#define C_CFISH_ERR

#include <setjmp.h>

#include "charmony.h"
#include "CFBind.h"

#include "Clownfish/Obj.h"
#include "Clownfish/Class.h"
#include "Clownfish/Blob.h"
#include "Clownfish/Boolean.h"
#include "Clownfish/ByteBuf.h"
#include "Clownfish/Err.h"
#include "Clownfish/Hash.h"
#include "Clownfish/HashIterator.h"
#include "Clownfish/Method.h"
#include "Clownfish/Num.h"
#include "Clownfish/String.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Util/Atomic.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Util/StringHelper.h"
#include "Clownfish/Vector.h"

static bool Err_initialized;

static PyTypeObject*
S_get_cached_py_type(cfish_Class *klass);

/**** Utility **************************************************************/

static bool
S_py_obj_is_a(PyObject *py_obj, cfish_Class *klass) {
    PyTypeObject *py_type = S_get_cached_py_type(klass);
    return !!PyObject_TypeCheck(py_obj, py_type);
}

void
CFBind_reraise_pyerr(cfish_Class *err_klass, cfish_String *mess) {
    PyObject *type, *value, *traceback;
    PyObject *type_pystr = NULL;
    PyObject *value_pystr = NULL;
    PyObject *traceback_pystr = NULL;
    char *type_str = "";
    char *value_str = "";
    char *traceback_str = "";
    PyErr_GetExcInfo(&type, &value, &traceback);
    if (type != NULL) {
        PyObject *type_pystr = PyObject_Str(type);
        type_str = PyUnicode_AsUTF8(type_pystr);
    }
    if (value != NULL) {
        PyObject *value_pystr = PyObject_Str(value);
        value_str = PyUnicode_AsUTF8(value_pystr);
    }
    if (traceback != NULL) {
        PyObject *traceback_pystr = PyObject_Str(traceback);
        traceback_str = PyUnicode_AsUTF8(traceback_pystr);
    }
    cfish_String *new_mess = cfish_Str_newf("%o... %s: %s %s", mess, type_str,
                                            value_str, traceback_str);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
    Py_XDECREF(type_pystr);
    Py_XDECREF(value_pystr);
    Py_XDECREF(traceback_pystr);
    CFISH_DECREF(mess);
    cfish_Err_throw_mess(err_klass, new_mess);
}

static cfish_Vector*
S_py_list_to_vector(PyObject *list) {
    Py_ssize_t size = PyList_GET_SIZE(list);
    cfish_Vector *vec = cfish_Vec_new(size);
    for (Py_ssize_t i = 0; i < size; i++) {
        CFISH_Vec_Store(vec, i, CFBind_py_to_cfish(PyList_GET_ITEM(list, i), NULL));
    }
    return vec;
}

static cfish_Hash*
S_py_dict_to_hash(PyObject *dict) {
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    cfish_Hash *hash = cfish_Hash_new(PyDict_Size(dict));
    while (PyDict_Next(dict, &pos, &key, &value)) {
        char *ptr;
        Py_ssize_t size;
        PyObject *stringified = key;
        if (!PyUnicode_CheckExact(key)) {
            stringified = PyObject_Str(key);
        }
        ptr = PyUnicode_AsUTF8AndSize(stringified, &size);
        if (!ptr) {
            cfish_String *mess
                = CFISH_MAKE_MESS("Failed to stringify as UTF-8");
            CFBind_reraise_pyerr(CFISH_ERR, mess);
        }
        CFISH_Hash_Store_Utf8(hash, ptr, size, CFBind_py_to_cfish(value, NULL));
        if (stringified != key) {
            Py_DECREF(stringified);
        }
    }
    return hash;
}

static cfish_Obj*
S_maybe_increment(void *vobj, bool increment) {
    if (increment) {
        return CFISH_INCREF((cfish_Obj*)vobj);
    }
    return (cfish_Obj*)vobj;
}

static bool
S_maybe_py_to_cfish(PyObject *py_obj, cfish_Class *klass, bool increment,
                    bool nullable, void *allocation, cfish_Obj **obj_ptr) {
    CFISH_UNUSED_VAR(allocation); // FIXME implement stack strings

    if (!py_obj || py_obj == Py_None) {
        *obj_ptr = NULL;
        return nullable;
    }

	// Default to accepting any type.
	if (klass == NULL) {
		klass = CFISH_OBJ;
	}

    if (S_py_obj_is_a(py_obj, klass)) {
        *obj_ptr = S_maybe_increment(py_obj, increment);
        return true;
    }
    else if (py_obj == Py_True) {
        if (klass != CFISH_BOOLEAN && klass != CFISH_OBJ) {
            return false;
        }
        *obj_ptr = S_maybe_increment(CFISH_TRUE, increment);
        return true;
    }
    else if (py_obj == Py_False) {
        if (klass != CFISH_BOOLEAN && klass != CFISH_OBJ) {
            return false;
        }
        *obj_ptr = S_maybe_increment(CFISH_FALSE, increment);
        return true;
    }
    else if (klass == CFISH_BOOLEAN) {
        int truthiness = PyObject_IsTrue(py_obj);
        if (truthiness == 1) {
            *obj_ptr = S_maybe_increment(CFISH_TRUE, increment);
        }
        else if (truthiness == 0) {
            *obj_ptr = S_maybe_increment(CFISH_FALSE, increment);
        }
        else {
            return false;
        }
        return true;
    }

    // From here on out, we're going to return a new Clownfish object.  The
    // caller has to take ownership of a refcount; if they don't want to, then
    // fail rather than attempt to return an object with a refcount of 0.
    if (!increment) {
        return false;
    }

    if (PyUnicode_CheckExact(py_obj)) {
        if (klass != CFISH_STRING && klass != CFISH_OBJ) {
            return false;
        }
        // TODO: Allow Clownfish String to wrap buffer of Python str?
        Py_ssize_t size;
        char *ptr = PyUnicode_AsUTF8AndSize(py_obj, &size);
        // TODO: Can we guarantee that Python will always supply valid UTF-8?
        if (!ptr || !cfish_StrHelp_utf8_valid(ptr, size)) {
            return false;
        }
        *obj_ptr = (cfish_Obj*)cfish_Str_new_from_trusted_utf8(ptr, size);
        return true;
    }
    else if (PyBytes_CheckExact(py_obj)) {
        if (klass != CFISH_BLOB && klass != CFISH_OBJ) {
            return false;
        }
        char *ptr = PyBytes_AS_STRING(py_obj);
        Py_ssize_t size = PyBytes_GET_SIZE(py_obj);
        *obj_ptr = (cfish_Obj*)cfish_BB_new_bytes(ptr, size);
        return true;
    }
    else if (PyList_CheckExact(py_obj)) {
        if (klass != CFISH_VECTOR && klass != CFISH_OBJ) {
            return false;
        }
        *obj_ptr = (cfish_Obj*)S_py_list_to_vector(py_obj);
        return true;
    }
    else if (PyDict_CheckExact(py_obj)) {
        if (klass != CFISH_HASH && klass != CFISH_OBJ) {
            return false;
        }
        *obj_ptr = (cfish_Obj*)S_py_dict_to_hash(py_obj);
        return true;
    }
    else if (PyLong_CheckExact(py_obj)) {
        if (klass != CFISH_INTEGER && klass != CFISH_OBJ) {
            return false;
        }
        // Raises ValueError on overflow.
        int64_t value = PyLong_AsLongLong(py_obj);
        if (PyErr_Occurred()) {
            return false;
        }
        *obj_ptr = (cfish_Obj*)cfish_Int_new(value);
        return true;
    }
    else if (PyFloat_CheckExact(py_obj)) {
        if (klass != CFISH_FLOAT && klass != CFISH_OBJ) {
            return false;
        }
        double value = PyFloat_AsDouble(py_obj);
        if (PyErr_Occurred()) {
            return false;
        }
        *obj_ptr = (cfish_Obj*)cfish_Float_new(value);
        return true;
    }

    // The value did not meet the required spec, so return false to indicate
    // failure.
    return false;
}

cfish_Obj*
CFBind_py_to_cfish_nullable(PyObject *py_obj, cfish_Class *klass) {
    cfish_Obj *retval;
    bool success = S_maybe_py_to_cfish(py_obj, klass, true, true, NULL, &retval);
    if (!success) {
        CFISH_THROW(CFISH_ERR, "Can't convert to %o", klass);
    }
    return retval;
}

cfish_Obj*
CFBind_py_to_cfish(PyObject *py_obj, cfish_Class *klass) {
    cfish_Obj *retval;
    bool success = S_maybe_py_to_cfish(py_obj, klass, true, false, NULL, &retval);
    if (!success) {
        CFISH_THROW(CFISH_ERR, "Can't convert to %o", klass);
    }
    return retval;
}

cfish_Obj*
CFBind_py_to_cfish_noinc(PyObject *py_obj, cfish_Class *klass,
                         void *allocation) {
    cfish_Obj *retval;
    bool success = S_maybe_py_to_cfish(py_obj, klass, false, false, allocation, &retval);
    if (!success) {
        CFISH_THROW(CFISH_ERR, "Can't convert to %o", klass);
    }
    return retval;
}

static int
S_convert_obj(PyObject *py_obj, CFBindArg *arg, bool nullable) {
    if (py_obj == Py_None) {
        if (nullable) {
            return 1;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }

    bool success = S_maybe_py_to_cfish(py_obj, arg->klass, true, nullable,
                                       NULL, arg->ptr);
    if (!success) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument type");
        return 0;
    }
    return 1;
}

int
CFBind_convert_obj(PyObject *py_obj, CFBindArg *arg) {
    return S_convert_obj(py_obj, arg, false);
}

int
CFBind_maybe_convert_obj(PyObject *py_obj, CFBindArg *arg) {
    return S_convert_obj(py_obj, arg, true);
}

static int
S_convert_string(PyObject *py_obj, cfish_String **ptr, bool nullable) {
    if (py_obj == NULL) { // Py_CLEANUP_SUPPORTED cleanup
        if (*ptr != NULL) {
            if (!CFISH_Str_Is_Copy_On_IncRef(*ptr)) {
                CFISH_DECREF(*ptr);
            }
            *ptr = NULL;
        }
        return 1;
    }

    if (py_obj == Py_None) {
        if (*ptr != NULL) {
            // Default value passed as stack string.
            *ptr = CFISH_Str_Clone(*ptr);
            return Py_CLEANUP_SUPPORTED;
        }
        else if (nullable) {
            return 1;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }
    else if (PyUnicode_CheckExact(py_obj)) {
        Py_ssize_t size;
        char *utf8 = PyUnicode_AsUTF8AndSize(py_obj, &size);
        if (!utf8) {
            return 0;
        }
        *ptr = cfish_Str_new_from_trusted_utf8(utf8, size);
        return Py_CLEANUP_SUPPORTED;
    }
    else if (S_py_obj_is_a(py_obj, CFISH_STRING)) {
        *ptr = (cfish_String*)CFISH_INCREF(py_obj);
        return Py_CLEANUP_SUPPORTED;
    }
    else if (S_py_obj_is_a(py_obj, CFISH_OBJ)) {
        *ptr = CFISH_Obj_To_String((cfish_Obj*)py_obj);
        return Py_CLEANUP_SUPPORTED;
    }
    else {
        PyObject *stringified = PyObject_Str(py_obj);
        if (stringified == NULL) {
            return 0;
        }
        Py_ssize_t size;
        char *utf8 = PyUnicode_AsUTF8AndSize(stringified, &size);
        if (!utf8) {
            Py_DECREF(stringified);
            return 0;
        }
        *ptr = cfish_Str_new_from_trusted_utf8(utf8, size);
        Py_DECREF(stringified);
        return Py_CLEANUP_SUPPORTED;
    }
}

int
CFBind_convert_string(PyObject *py_obj, cfish_String **ptr) {
    return S_convert_string(py_obj, ptr, false);
}

int
CFBind_maybe_convert_string(PyObject *py_obj, cfish_String **ptr) {
    return S_convert_string(py_obj, ptr, true);
}

static int
S_convert_hash(PyObject *py_obj, cfish_Hash **hash_ptr, bool nullable) {
    if (py_obj == NULL) { // Py_CLEANUP_SUPPORTED cleanup
        CFISH_DECREF(*hash_ptr);
        return 1;
    }

    if (py_obj == Py_None) {
        if (nullable) {
            return Py_CLEANUP_SUPPORTED;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }
    else if (PyDict_CheckExact(py_obj)) {
        *hash_ptr = S_py_dict_to_hash(py_obj);
        return Py_CLEANUP_SUPPORTED;
    }
    else if (S_py_obj_is_a(py_obj, CFISH_HASH)) {
        *hash_ptr = (cfish_Hash*)CFISH_INCREF(py_obj);
        return Py_CLEANUP_SUPPORTED;
    }
    else {
        return 0;
    }
}

int
CFBind_convert_hash(PyObject *py_obj, cfish_Hash **hash_ptr) {
    return S_convert_hash(py_obj, hash_ptr, false);
}

int
CFBind_maybe_convert_hash(PyObject *py_obj, cfish_Hash **hash_ptr) {
    return S_convert_hash(py_obj, hash_ptr, true);
}

static int
S_convert_vec(PyObject *py_obj, cfish_Vector **vec_ptr, bool nullable) {
    if (py_obj == NULL) { // Py_CLEANUP_SUPPORTED cleanup
        CFISH_DECREF(*vec_ptr);
        return 1;
    }

    if (py_obj == Py_None) {
        if (nullable) {
            return Py_CLEANUP_SUPPORTED;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }
    else if (PyList_CheckExact(py_obj)) {
        *vec_ptr = S_py_list_to_vector(py_obj);
        return Py_CLEANUP_SUPPORTED;
    }
    else if (S_py_obj_is_a(py_obj, CFISH_VECTOR)) {
        *vec_ptr = (cfish_Vector*)CFISH_INCREF(py_obj);
        return Py_CLEANUP_SUPPORTED;
    }
    else {
        return 0;
    }
}

int
CFBind_convert_vec(PyObject *py_obj, cfish_Vector **vec_ptr) {
    return S_convert_vec(py_obj, vec_ptr, false);
}

int
CFBind_maybe_convert_vec(PyObject *py_obj, cfish_Vector **vec_ptr) {
    return S_convert_vec(py_obj, vec_ptr, true);
}

static int
S_convert_sint(PyObject *py_obj, void *ptr, bool nullable, unsigned width) {
    if (py_obj == Py_None) {
        if (nullable) {
            return 1;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }
    int overflow = 0;
    int64_t value = PyLong_AsLongLongAndOverflow(py_obj, &overflow);
    if (value == -1 && PyErr_Occurred()) {
        return 0;
    }
    switch (width & 0xF) {
        case 1:
            if (value < INT8_MIN  || value > INT8_MAX)  { overflow = 1; }
            break;
        case 2:
            if (value < INT16_MIN || value > INT16_MAX) { overflow = 1; }
            break;
        case 4:
            if (value < INT32_MIN || value > INT32_MAX) { overflow = 1; }
            break;
        case 8:
            break;
    }
    if (overflow) {
        PyErr_SetString(PyExc_OverflowError, "Python int out of range");
        return 0;
    }
    switch (width & 0xF) {
        case 1:
            *((int8_t*)ptr) = (int8_t)value;
            break;
        case 2:
            *((int16_t*)ptr) = (int16_t)value;
            break;
        case 4:
            *((int32_t*)ptr) = (int32_t)value;
            break;
        case 8:
            *((int64_t*)ptr) = value;
            break;
    }
    return 1;
}

static int
S_convert_uint(PyObject *py_obj, void *ptr, bool nullable, unsigned width) {
    if (py_obj == Py_None) {
        if (nullable) {
            return 1;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }
    uint64_t value = PyLong_AsUnsignedLongLong(py_obj);
    if (PyErr_Occurred()) {
        return 0;
    }
    int overflow = 0;
    switch (width & 0xF) {
        case 1:
            if (value > UINT8_MAX)  { overflow = 1; }
            break;
        case 2:
            if (value > UINT16_MAX) { overflow = 1; }
            break;
        case 4:
            if (value > UINT32_MAX) { overflow = 1; }
            break;
        case 8:
            break;
    }
    if (overflow) {
        PyErr_SetString(PyExc_OverflowError, "Python int out of range");
        return 0;
    }
    switch (width & 0xF) {
        case 1:
            *((uint8_t*)ptr) = (uint8_t)value;
            break;
        case 2:
            *((uint16_t*)ptr) = (uint16_t)value;
            break;
        case 4:
            *((uint32_t*)ptr) = (uint32_t)value;
            break;
        case 8:
            *((uint64_t*)ptr) = value;
            break;
    }
    return 1;
}

int
CFBind_convert_char(PyObject *py_obj, char *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(char));
}

int
CFBind_convert_short(PyObject *py_obj, short *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(short));
}

int
CFBind_convert_int(PyObject *py_obj, int *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(int));
}

int
CFBind_convert_long(PyObject *py_obj, long *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(long));
}

int
CFBind_convert_int8_t(PyObject *py_obj, int8_t *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(int8_t));
}

int
CFBind_convert_int16_t(PyObject *py_obj, int16_t *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(int16_t));
}

int
CFBind_convert_int32_t(PyObject *py_obj, int32_t *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(int32_t));
}

int
CFBind_convert_int64_t(PyObject *py_obj, int64_t *ptr) {
    return S_convert_sint(py_obj, ptr, false, sizeof(int64_t));
}

int
CFBind_convert_uint8_t(PyObject *py_obj, uint8_t *ptr) {
    return S_convert_uint(py_obj, ptr, false, sizeof(uint8_t));
}

int
CFBind_convert_uint16_t(PyObject *py_obj, uint16_t *ptr) {
    return S_convert_uint(py_obj, ptr, false, sizeof(uint16_t));
}

int
CFBind_convert_uint32_t(PyObject *py_obj, uint32_t *ptr) {
    return S_convert_uint(py_obj, ptr, false, sizeof(uint32_t));
}

int
CFBind_convert_uint64_t(PyObject *py_obj, uint64_t *ptr) {
    return S_convert_uint(py_obj, ptr, false, sizeof(uint64_t));
}

int
CFBind_convert_size_t(PyObject *py_obj, size_t *ptr) {
    return S_convert_uint(py_obj, ptr, false, sizeof(size_t));
}

int
CFBind_maybe_convert_char(PyObject *py_obj, char *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(char));
}

int
CFBind_maybe_convert_short(PyObject *py_obj, short *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(short));
}

int
CFBind_maybe_convert_int(PyObject *py_obj, int *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(int));
}

int
CFBind_maybe_convert_long(PyObject *py_obj, long *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(long));
}

int
CFBind_maybe_convert_int8_t(PyObject *py_obj, int8_t *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(int8_t));
}

int
CFBind_maybe_convert_int16_t(PyObject *py_obj, int16_t *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(int16_t));
}

int
CFBind_maybe_convert_int32_t(PyObject *py_obj, int32_t *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(int32_t));
}

int
CFBind_maybe_convert_int64_t(PyObject *py_obj, int64_t *ptr) {
    return S_convert_sint(py_obj, ptr, true, sizeof(int64_t));
}

int
CFBind_maybe_convert_uint8_t(PyObject *py_obj, uint8_t *ptr) {
    return S_convert_uint(py_obj, ptr, true, sizeof(uint8_t));
}

int
CFBind_maybe_convert_uint16_t(PyObject *py_obj, uint16_t *ptr) {
    return S_convert_uint(py_obj, ptr, true, sizeof(uint16_t));
}

int
CFBind_maybe_convert_uint32_t(PyObject *py_obj, uint32_t *ptr) {
    return S_convert_uint(py_obj, ptr, true, sizeof(uint32_t));
}

int
CFBind_maybe_convert_uint64_t(PyObject *py_obj, uint64_t *ptr) {
    return S_convert_uint(py_obj, ptr, true, sizeof(uint64_t));
}

int
CFBind_maybe_convert_size_t(PyObject *py_obj, size_t *ptr) {
    return S_convert_uint(py_obj, ptr, true, sizeof(size_t));
}

static int
S_convert_floating(PyObject *py_obj, void *ptr, bool nullable, int width) {
    if (py_obj == Py_None) {
        if (nullable) {
            return 1;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }
    double value = PyFloat_AsDouble(py_obj);
    if (PyErr_Occurred()) {
        return 0;
    }
    switch (width & 0xF) {
        case sizeof(float):
            *((float*)ptr) = (float)value;
            break;
        case sizeof(double):
            *((double*)ptr) = value;
            break;
    }
    return 1;
}

int
CFBind_convert_float(PyObject *py_obj, float *ptr) {
    return S_convert_floating(py_obj, ptr, false, sizeof(float));
}

int
CFBind_convert_double(PyObject *py_obj, double *ptr) {
    return S_convert_floating(py_obj, ptr, false, sizeof(double));
}

int
CFBind_maybe_convert_float(PyObject *py_obj, float *ptr) {
    return S_convert_floating(py_obj, ptr, true, sizeof(float));
}

int
CFBind_maybe_convert_double(PyObject *py_obj, double *ptr) {
    return S_convert_floating(py_obj, ptr, true, sizeof(double));
}

static int
S_convert_bool(PyObject *py_obj, bool *ptr, bool nullable) {
    if (py_obj == Py_None) {
        if (nullable) {
            return 1;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Required argument cannot be None");
            return 0;
        }
    }
    int truth = PyObject_IsTrue(py_obj);
    if (truth == -1) {
        return 0;
    }
    *ptr = !!truth;
    return 1;
}

int
CFBind_convert_bool(PyObject *py_obj, bool *ptr) {
    return S_convert_bool(py_obj, ptr, false);
}

int
CFBind_maybe_convert_bool(PyObject *py_obj, bool *ptr) {
    return S_convert_bool(py_obj, ptr, true);
}

typedef struct ClassMapElem {
    cfish_Class **klass_handle;
    PyTypeObject *py_type;
} ClassMapElem;

typedef struct ClassMap {
    int32_t size;
    ClassMapElem *elems;
} ClassMap;

/* Before we can invoke methods on any Clownfish object safely, we must know
 * about its corresponding Python type object.  This association must be made
 * early in the bootstrapping process, which is tricky.  We can't use any
 * of Clownfish's convenient data structures yet!
 */
static ClassMap *klass_map = NULL;

static ClassMap*
S_revise_class_map(ClassMap *current, cfish_Class ***klass_handles,
                   PyTypeObject **py_types, int32_t num_items) {
    int32_t num_current = current ? current->size : 0;
    int32_t total = num_current + num_items;
    ClassMap *revised = (ClassMap*)malloc(sizeof(ClassMap));
    revised->elems = (ClassMapElem*)malloc(total * sizeof(ClassMapElem));
    if (current) {
        size_t size = num_current * sizeof(ClassMapElem);
        memcpy(revised->elems, current->elems, size);
    }
    for (int32_t i = 0; i < num_items; i++) {
        revised->elems[i + num_current].klass_handle = klass_handles[i];
        revised->elems[i + num_current].py_type      = py_types[i];
    }
    revised->size = total;
    return revised;
}

void
CFBind_assoc_py_types(cfish_Class ***klass_handles, PyTypeObject **py_types,
                      int32_t num_items) {
    while (1) {
        ClassMap *current = klass_map;
        ClassMap *revised = S_revise_class_map(current, klass_handles,
                                               py_types, num_items);
        if (cfish_Atomic_cas_ptr((void*volatile*)&klass_map, current, revised)) {
            if (current) {
                // TODO: Use read locking.  For now we have to leak this
                // memory to avoid memory errors in case another thread isn't
                // done reading it yet.

                //free(current->elems);
                //free(current);
            }
            break;
        }
        else {
            // Another thread beat us to it.  Try again.
            free(revised->elems);
            free(revised);
        }
    }
}

/**** refcounting **********************************************************/

uint32_t
cfish_get_refcount(void *vself) {
    return Py_REFCNT(vself);
}

cfish_Obj*
cfish_inc_refcount(void *vself) {
    Py_INCREF(vself);
    return (cfish_Obj*)vself;
}

uint32_t
cfish_dec_refcount(void *vself) {
    uint32_t modified_refcount = Py_REFCNT(vself);
    Py_DECREF(vself);
    return modified_refcount;
}

/**** Obj ******************************************************************/

void*
CFISH_Obj_To_Host_IMP(cfish_Obj *self) {
    return CFISH_INCREF(self);
}

/**** Class ****************************************************************/

/* Tell Python about the size of Clownfish objects, by copying
 * `cfclass->obj_alloc_size` into `pytype->tp_basicsize`.
 * **THIS MUST BE RUN BEFORE Class_Make_Obj IS CALLED** because under the
 * Python bindings, Clownfish uses Python object allocation internally.
 * Furthermore, `tp_basicsize` is supposed to be set before `PyType_Ready()`
 * is called.
 *
 * Ideally we would set `tp_basicsize` from within `Class_register_with_host`,
 * but it doesn't get called until too late.
 */
void
CFBind_class_bootstrap_hook1(cfish_Class *self) {
    PyTypeObject *py_type = S_get_cached_py_type(self);
    if (PyType_HasFeature(py_type, Py_TPFLAGS_READY)) {
        if (py_type->tp_basicsize != (Py_ssize_t)self->obj_alloc_size) {
            fprintf(stderr, "PyType for %s readied with wrong alloc size\n",
                    py_type->tp_name),
            exit(1);
        }
    }
    else {
        if (self->parent) {
            py_type->tp_base = S_get_cached_py_type(self->parent);
        }
        py_type->tp_basicsize = self->obj_alloc_size;
        if (PyType_Ready(py_type) < 0) {
            fprintf(stderr, "PyType_Ready failed for %s\n",
                    py_type->tp_name),
            exit(1);
        }
    }
}

/* Check the Class object for its associated PyTypeObject, which is stored in
 * `klass->host_type`.  If it is not there yet, search the class mapping and
 * cache it in the object.  Return the PyTypeObject.
 */
static PyTypeObject*
S_get_cached_py_type(cfish_Class *self) {
    PyTypeObject *py_type = (PyTypeObject*)self->host_type;
    if (py_type == NULL) {
        ClassMap *current = klass_map;
        for (int32_t i = 0; i < current->size; i++) {
            cfish_Class **handle = current->elems[i].klass_handle;
            if (handle == NULL || *handle != self) {
                continue;
            }
            py_type = current->elems[i].py_type;
            Py_INCREF(py_type);
            if (!cfish_Atomic_cas_ptr((void*volatile*)&self->host_type, py_type, NULL)) {
                // Lost the race to another thread, so get rid of the refcount.
                Py_DECREF(py_type);
            }
            break;
        }
    }
    if (py_type == NULL) {
        if (Err_initialized) {
            CFISH_THROW(CFISH_ERR,
                        "Can't find a Python type object corresponding to %o",
                        CFISH_Class_Get_Name(self));
        }
        else {
            fprintf(stderr, "Can't find a Python type corresponding to a "
                            "Clownfish class\n");
            exit(1);
        }
    }
    return py_type;
}

cfish_Obj*
CFISH_Class_Make_Obj_IMP(cfish_Class *self) {
    PyTypeObject *py_type = S_get_cached_py_type(self);
    cfish_Obj *obj = (cfish_Obj*)py_type->tp_alloc(py_type, 0);
    obj->klass = self;
    return obj;
}

cfish_Obj*
CFISH_Class_Init_Obj_IMP(cfish_Class *self, void *allocation) {
    PyTypeObject *py_type = S_get_cached_py_type(self);
    // It would be nice if we could call PyObject_Init() here and feel
    // confident that we have performed all Python-specific initialization
    // under all possible configurations, but that's not possible.  In
    // addition to setting ob_refcnt and ob_type, PyObject_Init() performs
    // tracking for heap allocated objects under special builds -- but
    // Class_Init_Obj() may be called on non-heap memory, such as
    // stack-allocated Clownfish Strings.  Therefore, we must perform a subset
    // of tasks selected from PyObject_Init() manually.
    cfish_Obj *obj = (cfish_Obj*)allocation;
    obj->ob_base.ob_refcnt = 1;
    obj->ob_base.ob_type = py_type;
    obj->klass = self;
    return obj;
}

void
cfish_Class_register_with_host(cfish_Class *singleton, cfish_Class *parent) {
    // FIXME
    CFISH_UNUSED_VAR(singleton);
    CFISH_UNUSED_VAR(parent);
}

cfish_Vector*
cfish_Class_fresh_host_methods(cfish_String *class_name) {
    // FIXME Scan Python class for host methods which override.  Until this is
    // implemented, it will be impossible to override Clownfish methods from
    // Python.
    CFISH_UNUSED_VAR(class_name);
    return cfish_Vec_new(0);
}

cfish_String*
cfish_Class_find_parent_class(cfish_String *class_name) {
    // FIXME Until this is implemented, subclassing from Python will be
    // impossible.
    CFISH_UNUSED_VAR(class_name);
    return NULL;
}

/**** Method ***************************************************************/

cfish_String*
CFISH_Method_Host_Name_IMP(cfish_Method *self) {
    return cfish_Method_lower_snake_alias(self);
}

/**** Err ******************************************************************/

/* TODO: Thread safety? */
static cfish_Err *current_error;
static cfish_Err *thrown_error;
static jmp_buf  *current_env;

jmp_buf*
CFBind_swap_env(jmp_buf *env) {
    jmp_buf *prev_env = current_env;
    current_env = env;
    return prev_env;
}

int
CFBind_migrate_cferr() {
    if (thrown_error != NULL) {
        cfish_Err *err = thrown_error;
        thrown_error = NULL;
        cfish_String *mess = CFISH_Err_Get_Mess(err);
        char *utf8 = CFISH_Str_To_Utf8(mess);
        PyErr_SetString(PyExc_RuntimeError, utf8);
        CFISH_FREEMEM(utf8);
        CFISH_DECREF(err);
        return true;
    }
    return false;
}

void
cfish_Err_init_class(void) {
    Err_initialized = true;
}

cfish_Err*
cfish_Err_get_error() {
    return current_error;
}

void
cfish_Err_set_error(cfish_Err *error) {
    if (current_error) {
        CFISH_DECREF(current_error);
    }
    current_error = error;
}

void
cfish_Err_do_throw(cfish_Err *error) {
    if (current_env) {
        thrown_error = error;
        longjmp(*current_env, 1);
    }
    else {
        cfish_String *message = CFISH_Err_Get_Mess(error);
        char *utf8 = CFISH_Str_To_Utf8(message);
        fprintf(stderr, "%s", utf8);
        CFISH_FREEMEM(utf8);
        exit(EXIT_FAILURE);
    }
}

void
cfish_Err_throw_mess(cfish_Class *klass, cfish_String *message) {
    CFISH_UNUSED_VAR(klass); // TODO use klass
    cfish_Err *err = cfish_Err_new(message);
    cfish_Err_do_throw(err);
}

void
cfish_Err_warn_mess(cfish_String *message) {
    char *utf8 = CFISH_Str_To_Utf8(message);
    fprintf(stderr, "%s", utf8);
    CFISH_FREEMEM(utf8);
    CFISH_DECREF(message);
}

cfish_Err*
cfish_Err_trap(CFISH_Err_Attempt_t routine, void *context) {
    jmp_buf  env;
    jmp_buf *prev_env = current_env;
    current_env = &env;

    if (!setjmp(env)) {
        routine(context);
    }

    current_env = prev_env;

    cfish_Err *error = thrown_error;
    thrown_error = NULL;
    return error;
}

/**** TestUtils ************************************************************/

void*
cfish_TestUtils_clone_host_runtime() {
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void
cfish_TestUtils_set_host_runtime(void *runtime) {
    CFISH_UNUSED_VAR(runtime);
    CFISH_THROW(CFISH_ERR, "TODO");
}

void
cfish_TestUtils_destroy_host_runtime(void *runtime) {
    CFISH_UNUSED_VAR(runtime);
    CFISH_THROW(CFISH_ERR, "TODO");
}

/**** To_Host methods ******************************************************/

void*
CFISH_Str_To_Host_IMP(cfish_String *self) {
    const char *ptr = CFISH_Str_Get_Ptr8(self);
    size_t size = CFISH_Str_Get_Size(self);
    return PyUnicode_FromStringAndSize(ptr, size);
}

void*
CFISH_Blob_To_Host_IMP(cfish_Blob *self) {
    const char *buf = CFISH_Blob_Get_Buf(self);
    size_t size = CFISH_Blob_Get_Size(self);
    return PyBytes_FromStringAndSize(buf, size);
}

void*
CFISH_BB_To_Host_IMP(cfish_ByteBuf *self) {
    CFISH_BB_To_Host_t super_to_host
        = CFISH_SUPER_METHOD_PTR(CFISH_BYTEBUF, CFISH_BB_To_Host);
    return super_to_host(self);
}

void*
CFISH_Vec_To_Host_IMP(cfish_Vector *self) {
    uint32_t num_elems = CFISH_Vec_Get_Size(self);
    PyObject *list = PyList_New(num_elems);

    // Iterate over vector items.
    for (uint32_t i = 0; i < num_elems; i++) {
        cfish_Obj *val = CFISH_Vec_Fetch(self, i);
        PyObject *item = CFBind_cfish_to_py(val);
        PyList_SET_ITEM(list, i, item);
    }

    return list;
}

void*
CFISH_Hash_To_Host_IMP(cfish_Hash *self) {
    PyObject *dict = PyDict_New();

    // Iterate over key-value pairs.
    cfish_HashIterator *iter = cfish_HashIter_new(self);
    while (CFISH_HashIter_Next(iter)) {
        cfish_String *key = (cfish_String*)CFISH_HashIter_Get_Key(iter);
        if (!cfish_Obj_is_a((cfish_Obj*)key, CFISH_STRING)) {
            CFISH_THROW(CFISH_ERR, "Non-string key: %o",
                        cfish_Obj_get_class_name((cfish_Obj*)key));
        }
        size_t size = CFISH_Str_Get_Size(key);
        const char *ptr = CFISH_Str_Get_Ptr8(key);
        PyObject *py_key = PyUnicode_FromStringAndSize(ptr, size);
        PyObject *py_val = CFBind_cfish_to_py(CFISH_HashIter_Get_Value(iter));
        PyDict_SetItem(dict, py_key, py_val);
        Py_DECREF(py_key);
        Py_DECREF(py_val);
    }
    CFISH_DECREF(iter);

    return dict;
}

void*
CFISH_Float_To_Host_IMP(cfish_Float *self) {
    return PyFloat_FromDouble(CFISH_Float_Get_Value(self));
}

void*
CFISH_Int_To_Host_IMP(cfish_Integer *self) {
    int64_t num = CFISH_Int_Get_Value(self);
    return PyLong_FromLongLong(num);
}

void*
CFISH_Bool_To_Host_IMP(cfish_Boolean *self) {
    if (self == CFISH_TRUE) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}


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
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Vector.h"

static bool Err_initialized;

/**** Utility **************************************************************/

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

cfish_Obj*
CFISH_Class_Make_Obj_IMP(cfish_Class *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Obj*);
}

cfish_Obj*
CFISH_Class_Init_Obj_IMP(cfish_Class *self, void *allocation) {
    CFISH_UNUSED_VAR(self);
    CFISH_UNUSED_VAR(allocation);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Obj*);
}

void
cfish_Class_register_with_host(cfish_Class *singleton, cfish_Class *parent) {
    CFISH_UNUSED_VAR(singleton);
    CFISH_UNUSED_VAR(parent);
    CFISH_THROW(CFISH_ERR, "TODO");
}

cfish_Vector*
cfish_Class_fresh_host_methods(cfish_String *class_name) {
    CFISH_UNUSED_VAR(class_name);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_Vector*);
}

cfish_String*
cfish_Class_find_parent_class(cfish_String *class_name) {
    CFISH_UNUSED_VAR(class_name);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_String*);
}

/**** Method ***************************************************************/

cfish_String*
CFISH_Method_Host_Name_IMP(cfish_Method *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(cfish_String*);
}

/**** Err ******************************************************************/

/* TODO: Thread safety? */
static cfish_Err *current_error;
static cfish_Err *thrown_error;
static jmp_buf  *current_env;

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


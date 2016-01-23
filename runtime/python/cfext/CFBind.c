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
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
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
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Blob_To_Host_IMP(cfish_Blob *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_BB_To_Host_IMP(cfish_ByteBuf *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Vec_To_Host_IMP(cfish_Vector *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Hash_To_Host_IMP(cfish_Hash *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Float_To_Host_IMP(cfish_Float *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Int_To_Host_IMP(cfish_Integer *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}

void*
CFISH_Bool_To_Host_IMP(cfish_Boolean *self) {
    CFISH_UNUSED_VAR(self);
    CFISH_THROW(CFISH_ERR, "TODO");
    CFISH_UNREACHABLE_RETURN(void*);
}


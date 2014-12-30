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

#include "Python.h"
#include "CFC.h"

typedef struct {
    PyObject_HEAD
    void *cfc_obj;
} CFCPyWrapper;

static PyModuleDef cfc_module_def = {
    PyModuleDef_HEAD_INIT,
    "cfc._cfc",
    "CFC: Clownfish compiler",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

static PyModuleDef cfc_model_module_def = {
    PyModuleDef_HEAD_INIT,
    "cfc.model",
    "CFC classes which model language constructs",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

static PyModuleDef cfc_binding_module_def = {
    PyModuleDef_HEAD_INIT,
    "cfc.binding",
    "CFC components which generate bindings",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

static PyObject *cfc_module;
static PyObject *cfc_model_module;
static PyObject *cfc_binding_module;

static PyObject*
S_wrap_cfcbase(PyTypeObject *type, void *cfc_obj) {
    if (cfc_obj == NULL) {
        Py_RETURN_NONE;
    }
    CFCPyWrapper *wrapper = (CFCPyWrapper*)type->tp_alloc(type, 0);
    if (wrapper == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to allocate object");
        return NULL;
    }
    wrapper->cfc_obj = cfc_obj;
    return (PyObject*)wrapper;
}

static void
S_CFCBase_dealloc(CFCPyWrapper *wrapper) {
    CFCBase *temp = (CFCBase*)wrapper->cfc_obj;
    wrapper->cfc_obj = NULL;
    CFCBase_decref(temp);
    Py_TYPE(wrapper)->tp_free(wrapper);
}

void*
S_to_cfc_something(PyObject *wrapper, const char *class_name) {
    if (wrapper == NULL || wrapper == Py_None) {
        return NULL;
    }
    CFCBase *obj = (CFCBase*)((CFCPyWrapper*)wrapper)->cfc_obj;
    if (strcmp(CFCBase_get_cfc_class(obj), class_name) != 0) {
        CFCUtil_die("Object is not a %s, it's a %s", class_name,
                    CFCBase_get_cfc_class(obj));
    }
    return obj;
}

static CFCHierarchy*
S_to_Hierarchy(PyObject *wrapper) {
    return (CFCHierarchy*)S_to_cfc_something(wrapper,
        "Clownfish::CFC::Model::Hierarchy");
}

static CFCParcel*
S_to_Parcel(PyObject *wrapper) {
    return (CFCParcel*)S_to_cfc_something(wrapper,
        "Clownfish::CFC::Model::Parcel");
}

static CFCBindCore*
S_to_BindCore(PyObject *wrapper) {
    return (CFCBindCore*)S_to_cfc_something(wrapper,
        "Clownfish::CFC::Binding::Core");
}

static PyTypeObject *Hierarchy_pytype;
static PyTypeObject *Parcel_pytype;
static PyTypeObject *BindCore_pytype;

/***************************** CFCHierarchy *****************************/

static PyObject*
S_CFCHierarchy_new(PyTypeObject *type, PyObject *args,
                   PyObject *keyword_args) {
    char *dest = NULL;
    char *keywords[] = {"dest", NULL};
    int result = PyArg_ParseTupleAndKeywords(args, keyword_args, "|s",
                                             keywords, &dest);
    if (!result) { return NULL; }
    if (!dest) {
        PyErr_SetString(PyExc_TypeError, "Missing required arg 'dest'");
        return NULL;
    }
    PyObject *inc_func = PyObject_GetAttrString(cfc_module, "_get_inc_dirs");
    PyObject *dirs = NULL;
    if (PyCallable_Check(inc_func)) {
        dirs = PyObject_CallObject(inc_func, NULL);
    }
    if (dirs == NULL || !PyList_Check(dirs)) {
        Py_XDECREF(inc_func);
        Py_XDECREF(dirs);
        PyErr_SetString(PyExc_RuntimeError, "_get_inc_dirs failed");
        return NULL;
    }
    CFCHierarchy *hierarchy = CFCHierarchy_new(dest);
    for (Py_ssize_t i = 0, max = PyList_Size(dirs); i < max; i++) {
        PyObject *dir = PyList_GetItem(dirs, i);
        if (PyUnicode_Check(dir)) {
            char *dir_utf8 = PyUnicode_AsUTF8(dir);
            CFCHierarchy_add_include_dir(hierarchy, dir_utf8);
        }
        else {
            PyErr_SetString(PyExc_RuntimeError, "not a string");
            return NULL;
        }
    }

    Py_XDECREF(inc_func);
    Py_XDECREF(dirs);

    return S_wrap_cfcbase(Hierarchy_pytype, hierarchy);
}

static PyObject*
S_CFCHierarchy_build(PyObject *wrapper, PyObject *unused) {
    CHY_UNUSED_VAR(unused);
    CFCHierarchy_build(S_to_Hierarchy(wrapper));
    Py_RETURN_NONE;
}

static PyObject*
S_CFCHierarchy_add_include_dir(PyObject *wrapper, PyObject *dir) {
    CFCHierarchy_add_include_dir(S_to_Hierarchy(wrapper),
                                 PyUnicode_AsUTF8(dir));
    Py_RETURN_NONE;
}

static PyObject*
S_CFCHierarchy_add_source_dir(PyObject *wrapper, PyObject *dir) {
    CFCHierarchy_add_source_dir(S_to_Hierarchy(wrapper),
                                PyUnicode_AsUTF8(dir));
    Py_RETURN_NONE;
}

static PyObject*
S_CFCHierarchy_write_log(PyObject *wrapper, PyObject *unused) {
    CHY_UNUSED_VAR(unused);
    CFCHierarchy_write_log(S_to_Hierarchy(wrapper));
    Py_RETURN_NONE;
}

static PyMethodDef hierarchy_methods[] = {
    {"add_include_dir", (PyCFunction)S_CFCHierarchy_add_include_dir, METH_O,      NULL},
    {"add_source_dir",  (PyCFunction)S_CFCHierarchy_add_source_dir,  METH_O,      NULL},
    {"build",           (PyCFunction)S_CFCHierarchy_build,           METH_NOARGS, NULL},
    {"write_log",       (PyCFunction)S_CFCHierarchy_write_log,       METH_NOARGS, NULL},
    {NULL}
};

static PyTypeObject Hierarchy_pytype_struct = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cfc.model.Hierarchy",              // tp_name
    sizeof(CFCPyWrapper),               // tp_basicsize
    0,                                  // tp_itemsize
    (destructor)S_CFCBase_dealloc,      // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_reserved
    0,                                  // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    "CFCHierarchy",                     // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    hierarchy_methods,                  // tp_methods
    0,                                  // tp_members
    0,                                  // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    0,                                  // tp_init
    0,                                  // tp_allow
    (newfunc)S_CFCHierarchy_new         // tp_new
};

/***************************** CFCParcel *****************************/

PyObject*
S_CFCParcel_fetch(PyObject *ignored, PyObject *name) {
    CFCParcel *parcel = CFCParcel_fetch(PyUnicode_AsUTF8(name));
    return S_wrap_cfcbase(Parcel_pytype, parcel);
}

static PyMethodDef parcel_methods[] = {
    {"fetch", (PyCFunction)S_CFCParcel_fetch, METH_STATIC, NULL},
    {NULL}
};

static PyTypeObject Parcel_pytype_struct = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cfc.model.Parcel",                 // tp_name
    sizeof(CFCPyWrapper),               // tp_basicsize
    0,                                  // tp_itemsize
    (destructor)S_CFCBase_dealloc,      // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_reserved
    0,                                  // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    "CFCParcel",                        // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    parcel_methods,                     // tp_methods
    0,                                  // tp_members
    0,                                  // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    0,                                  // tp_init
    0,                                  // tp_allow
    0                                   // tp_new
};

/***************************** CFCBindCore *****************************/

static PyObject*
S_CFCBindCore_new(PyTypeObject *type, PyObject *args,
                   PyObject *keyword_args) {
    PyObject *hierarchy_wrapped;
    char *header = "";
    char *footer = "";
    char *keywords[] = {"hierarchy", "header", "footer", NULL};
    int result = PyArg_ParseTupleAndKeywords(args, keyword_args, "O!|ss",
                                             keywords, Hierarchy_pytype,
                                             &hierarchy_wrapped, &header,
                                             &footer);
    if (!result) { return NULL; }
    CFCHierarchy *hierarchy = S_to_Hierarchy(hierarchy_wrapped);
    CFCBindCore *obj = CFCBindCore_new(hierarchy, header, footer);
    return S_wrap_cfcbase(BindCore_pytype, obj);
}

static PyObject*
S_CFCBindCore_write_all_modified(PyObject *wrapper, PyObject *args) {
    int modified = 0;
    int result = PyArg_ParseTuple(args, "|p", &modified);
    if (!result) { return NULL; }
    CFCBindCore_write_all_modified(S_to_BindCore(wrapper), modified);
    Py_RETURN_NONE;
}

static PyMethodDef bindcore_methods[] = {
    {"write_all_modified", (PyCFunction)S_CFCBindCore_write_all_modified, METH_VARARGS, NULL},
    {NULL}
};

static PyTypeObject BindCore_pytype_struct = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cfc.binding.BindCore",             // tp_name
    sizeof(CFCPyWrapper),               // tp_basicsize
    0,                                  // tp_itemsize
    (destructor)S_CFCBase_dealloc,      // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_reserved
    0,                                  // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    "CFCBindCore",                      // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    bindcore_methods,                   // tp_methods
    0,                                  // tp_members
    0,                                  // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    0,                                  // tp_init
    0,                                  // tp_allow
    (newfunc)S_CFCBindCore_new          // tp_new
};

/******************************* common ******************************/

PyMODINIT_FUNC
PyInit__cfc(void) {
    // Initialize Python type objects.
    Hierarchy_pytype = &Hierarchy_pytype_struct;
    Parcel_pytype    = &Parcel_pytype_struct;
    BindCore_pytype  = &BindCore_pytype_struct;
    if (PyType_Ready(Hierarchy_pytype) < 0) {
        return NULL;
    }
    if (PyType_Ready(Parcel_pytype) < 0) {
        return NULL;
    }
    if (PyType_Ready(BindCore_pytype) < 0) {
        return NULL;
    }

    // Initialize modules.
    cfc_module = PyModule_Create(&cfc_module_def);
    cfc_model_module = PyModule_Create(&cfc_model_module_def);
    cfc_binding_module = PyModule_Create(&cfc_binding_module_def);
    PyModule_AddObject(cfc_module, "model", (PyObject*)cfc_model_module);
    PyModule_AddObject(cfc_module, "binding", (PyObject*)cfc_binding_module);

    // Add type objects to "model" module.
    Py_INCREF(Hierarchy_pytype);
    PyModule_AddObject(cfc_model_module, "Hierarchy",
                       (PyObject*)Hierarchy_pytype);
    Py_INCREF(Parcel_pytype);
    PyModule_AddObject(cfc_model_module, "Parcel",
                       (PyObject*)Parcel_pytype);

    // Add type objects to "binding" module.
    Py_INCREF(BindCore_pytype);
    PyModule_AddObject(cfc_binding_module, "BindCore",
                       (PyObject*)BindCore_pytype);

    return cfc_module;
}



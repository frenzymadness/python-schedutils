/*
   Python 3 compatibility macros
   Copyright (C) Petr Viktorin <pviktori@redhat.com> 2015
*/

#include <Python.h>

#if PY_MAJOR_VERSION >= 3

/***** Python 3 *****/

#define IS_PY3 1

/* Ints */

#define PyInt_AsLong PyLong_AsLong

/* Strings */

#define PyStr_FromString PyUnicode_FromString

/* Module init */

#define MODULE_INIT_FUNC(name) \
    PyMODINIT_FUNC PyInit_ ## name(void); \
    PyMODINIT_FUNC PyInit_ ## name(void)

#else

/***** Python 2 *****/

#define IS_PY3 0

/* Strings */

#define PyStr_FromString PyString_FromString

/* Module init */

#define PyModuleDef_HEAD_INIT 0

typedef struct PyModuleDef {
    int m_base;
    const char* m_name;
    const char* m_doc;
    Py_ssize_t m_size;
    PyMethodDef *m_methods;
} PyModuleDef;

#define PyModule_Create(def) \
    Py_InitModule3((def)->m_name, (def)->m_methods, (def)->m_doc)

#define MODULE_INIT_FUNC(name) \
    static PyObject *PyInit_ ## name(void); \
    void init ## name(void); \
    void init ## name(void) { PyInit_ ## name(); } \
    static PyObject *PyInit_ ## name(void)


#endif

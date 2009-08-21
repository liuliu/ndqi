%module ndqi

%typemap(newfree) TCMAP* {
    tcmapdel($1);
}

%typemap(out) TCMAP* {
    tcmapiterinit($1);
    int sp;
    PyObject* dict = PyDict_New();
    char* key = (char*)tcmapiternext($1, &sp);
    while (key != NULL)
    {
        int siz;
        char* val = (char*)tcmapget($1, key, sp, &siz);
        if (val != NULL)
        {
            PyObject* pyKey = PyString_FromString(key);
            PyObject* pyVal = PyString_FromString(val);
            PyDict_SetItem(dict, pyKey, pyVal);
            Py_DECREF(pyKey);
            Py_DECREF(pyVal);
        }
        key = (char*)tcmapiternext($1, &sp);
    }
    $result = dict;
}

%{
#define SWIG_FILE_WITH_INIT
#include "../nqmeta.h"
%}

%newobject nqmetanew;
TCMAP* nqmetanew(const char* file);

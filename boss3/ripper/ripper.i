%module ripper
%{
#include <Python.h>
#include "../metadata/id3.h"
%}
PyObject *getCDDB (char *device);
int pyrip (char *device, PyObject *cfgparser, PyObject *tags, PyObject *filenames);
PyObject *pyrip_update (void);

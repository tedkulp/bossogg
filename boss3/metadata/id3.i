%module id3
%{
#include <Python.h>
#include "id3.h"
%}
PyObject *getTag (char *filename);
text_tag_s *setTag (PyObject *tag);

%module vorbis
%{
#include <Python.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
%}
PyObject* getMetaData(char* filename);

if OGG_OPT
USE_OGG=vorbis.c
endif

if ID3_OPT
USE_ID3=id3lib_wrapper.cc id3lib_wrapper.h
endif

python_PYTHON=__init__.py id3.py

python_LTLIBRARIES=_id3.la

_id3_la_LDFLAGS=-module @METADATA_LIBS@
_id3_la_CFLAGS=-I../bossao
_id3_la_CXXFLAGS=-I../bossao
_id3_la_SOURCES=id3.h id3.c id3_wrap.c ${USE_OGG} ${USE_ID3} 

id3_wrap.c: 
	swig -includeall -python id3.i

all-local: _id3.so

clean-local:
	rm _id3.so _id3.la

_id3.so: .libs/_id3.so
	cp .libs/_id3.so .

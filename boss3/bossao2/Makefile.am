if OGG_OPT
OGG_LIB=input_ogg.la
input_ogg_la_LDFLAGS=-module
input_ogg_la_SOURCES=input_ogg.c
endif

if MP3_OPT
MP3_LIB=input_mp3.la
input_mp3_la_LDFLAGS=-module
input_mp3_la_SOURCES=input_mp3.c
endif

if FLAC_OPT
FLAC_LIB=input_flac.la
input_flac_la_LDFLAGS=-module
input_flac_la_SOURCES=input_flac.c
endif

if SHOUT_OPT
USE_SHOUT=shout.c
endif

CFLAGS+=-Wall -ggdb -pthread -Wno-import -fPIC -Iinclude `pkg-config glib-2.0 --cflags`
LDFLAGS += `pkg-config glib-2.0 gthread-2.0 gmodule-2.0 --libs`

python_PYTHON=__init__.py bossao.py
noinst_PROGRAMS=test

test_SOURCES = test.c 

python_LTLIBRARIES=_bossao.la output_ao.la output_file.la $(MP3_LIB) $(OGG_LIB) $(FLAC_LIB)

output_ao_la_LDFLAGS=-module
output_ao_la_SOURCES=output_ao.c

output_file_la_LDFLAGS=-module
output_file_la_SOURCES=output_file.c

_bossao_la_LDFLAGS=-module
_bossao_la_SOURCES=bossao.c thbuf.c output_plugin.c input_plugin.c #bossao_wrap.c

all-local: _bossao.so

clean-local:
	rm -f *~ *.pyc bossao_wrap.c _bossao.so _bossao.la

bossao_wrap.c: bossao.i
	swig -includeall -python bossao.i

test: test.c thbuf.o bossao.o input_plugin.o output_plugin.o
	gcc $(CFLAGS) -I../.. -o test test.c thbuf.o output_plugin.o input_plugin.o bossao.o -lutil -lpthread -lao -logg -lvorbis -lvorbisfile -lvorbisenc -lFLAC -lshout -lmad -lpython2.3 `pkg-config glib-2.0 gmodule-2.0 gthread-2.0 --libs`

_bossao.so: .libs/_bossao.so
	cp .libs/_bossao.so .

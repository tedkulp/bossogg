if OGG_OPT
USE_OGG=ogg.c ogg.h
endif

if MP3_OPT
USE_MP3=mp3.c mp3.h
endif

if FLAC_OPT
USE_FLAC=flac.c flac.h
endif

if OSS_MIXER_OPT
USE_OSS_MIXER=oss_mixer.c oss_mixer.h
endif

if SHOUT_OPT
USE_SHOUT=shout.c shout.h
endif

CFLAGS+=-Wall -ggdb -pthread

python_PYTHON=__init__.py bossao.py
noinst_PROGRAMS=test


test_SOURCES = test.c 

python_LTLIBRARIES=_bossao.la

_bossao_la_LDFLAGS=-module
_bossao_la_SOURCES=bossao.c thbuf.c bossao_wrap.c $(USE_SHOUT) $(USE_OGG) $(USE_FLAC) ${USE_MP3} $(USE_OSS_MIXER)

all-local: _bossao.so

clean-local:
	rm -f *~ *.pyc bossao_wrap.c _bossao.so _bossao.la

bossao_wrap.c: bossao.i
	swig -includeall -python bossao.i

test: test.c thbuf.o bossao_wrap.o shout.o ogg.o flac.o mp3.o bossao.o oss_mixer.o
	gcc $(CFLAGS) -I../.. -o test test.c thbuf.o oss_mixer.o bossao_wrap.o shout.o ogg.o flac.o mp3.o bossao.o -lutil -lpthread -lao -logg -lvorbis -lvorbisfile -lvorbisenc -lFLAC -lshout -lmad -lpython2.3

_bossao.so: .libs/_bossao.so
	cp .libs/_bossao.so .

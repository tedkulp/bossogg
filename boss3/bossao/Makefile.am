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

python_PYTHON=__init__.py bossao.py

python_LTLIBRARIES=_bossao.la

_bossao_la_LDFLAGS=-module
_bossao_la_SOURCES=bossao.h bossao.c bossao_wrap.c $(USE_SHOUT) $(USE_OGG) $(USE_FLAC) ${USE_MP3} $(USE_OSS_MIXER)

all-local: _bossao.so

clean-local:
	rm _bossao.so _bossao.la

#bossao_wrap.c: bossao.i
bossao_wrap.c:
	swig -includeall -python bossao.i

_bossao.so: .libs/_bossao.so
	cp .libs/_bossao.so .

CC = gcc
CFLAGS = -ggdb -I/usr/include/python2.3 
LDLIBS = -ggdb -L/usr/lib/python2.3/config -lz -lpython2.3 -lcdda_interface -lcdda_paranoia -lcddb -lFLAC -lid3 -logg -lvorbis -lvorbisfile -lvorbisenc -lpthread -ldl -lutil

all: ripper.py _ripper.so test-ripper

ripper.py: ripper.i
	swig -v -includeall -python ripper.i

test-ripper: test-ripper.o rip.o cddb.o ../metadata/_id3.so 

test-ripper.o: test-ripper.c

_ripper.so: ripper_wrap.o rip.o cddb.o
	gcc -shared -o _ripper.so ../metadata/_id3.so cddb.o rip.o ripper_wrap.o ${LDLIBS}

ripper_wrap.o: ripper_wrap.c

cddb.o: cddb.c rip.h config.h ../metadata/id3.h 

rip.o: rip.c rip.h config.h ../metadata/id3.h

clean:
	rm -f *.o *.so *.pyc ripper.py ripper_wrap.c *~ test-ripper

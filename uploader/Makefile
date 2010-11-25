CFLAGS = -Wall -g -I. -I../
LDFLAGS = -lportmidi -lporttime 
CC = g++
SRC = midibytepack.cpp \
		main.cpp

OBJ = ${SRC:.cpp=.o}

current: sysexupload

.cpp.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $*.o $<

sysexupload: ${OBJ}
	@echo CC $@
	@${CC} ${CFLAGS} -o $@ ${OBJ} ${LDFLAGS}

clean: 
	rm -f *.o sysexupload

all: sysexupload

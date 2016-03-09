.SUFFIXES: .o .c

SRC = farbfeld.c
OBJ = ${SRC:.c=.o}
INCLUDES := $(shell pkg-config --cflags gimp-2.0)
LIBS := $(shell pkg-config --libs gimp-2.0)
GIMP_VERSION := $(basename $(shell pkg-config --modversion gimp-2.0))
INSTALL_DIR = ${HOME}/.gimp-${GIMP_VERSION}/plug-ins
all: farbfeld

.c.o:
	${CC} ${INCLUDES} ${CFLAGS} -o $@ -c $<

farbfeld: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS} ${LDFLAGS}

clean:
	rm -f farbfeld ${OBJ}

install: farbfeld
	cp farbfeld ${INSTALL_DIR}

uninstall:
	rm -f ${INSTALL_DIR}/farbfeld

.PHONY: all clean install

include config.mk

.SUFFIXES: .o .c

SRC = farbfeld.c
OBJ = ${SRC:.c=.o}

all: farbfeld

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

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

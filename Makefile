PREFIX = /usr/local
LIBS = -lgit2 -lcurl -ljson-c
FLAGS = ${LIBS}
PROG = aureate

all:
	cc ${PROG}.c ${FLAGS} -o ${PROG}

debug:
	clang ${PROG}.c -Wall -Werror -fsanitize=undefined,address ${FLAGS} -o ${PROG}

install: all
	install -Dm755 ./${PROG} ${PREFIX}/bin/${PROG}
	install -Dm644 ./${PROG}.1 ${PREFIX}/share/man/man1/${PROG}.1

uninstall:
	rm -f ${PREFIX}/bin/${PROG}
	rm -f ${PREFIX}/share/man/man1/${PROG}.1

clean:
	rm -rf ${XDG_CACHE_HOME}/${PROG}

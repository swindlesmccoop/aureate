PREFIX = /usr/local
LIBS = -lgit2 -lcurl -ljson-c
FLAGS = ${LIBS}
PROG = aureate

all:
	cc ${PROG}.c ${FLAGS} -o ${PROG}

debug:
	clang ${PROG} -Wall -Werror -fsanitize=undefined,address ${FLAGS}

install:
	install -Dm755 ./${PROG} ${PREFIX}/bin/${PROG}

uninstall:
	rm -f ${PREFIX}/bin/${PROG}

clean:
	rm -rf ${XDG_CACHE_HOME}/${PROG}
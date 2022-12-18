PREFIX = /usr/local
LIBS = -lgit2 -lcurl
FLAGS = ${LIBS} -o aureate

all:
	cc aureate.c ${FLAGS}

verbose:
	clang aureate.c -Wall -Werror -fsanitize=undefined,address ${FLAGS}

install:
	install -Dm755 ./aureate ${PREFIX}/bin/aureate

uninstall:
	rm -f ${PREFIX}/bin/aureate

clean:
	rm -rf ${XDG_CACHE_HOME}/aureate
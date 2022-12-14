PREFIX = /usr/local

all:
	cc aureate.c -lcurl -larchive -o aureate

verbose:
	clang aureate.c -Wall -Werror -fsanitize=undefined,address -lcurl -larchive -o aureate

install:
	install -Dm755 ./aureate ${PREFIX}/bin/aureate
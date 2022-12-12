PREFIX = /usr/local

all:
	cc aureate.c -Wall -Werror -fsanitize=undefined,address -lcurl -o aureate

install:
	install -Dm755 ./aureate ${PREFIX}/bin/aureate
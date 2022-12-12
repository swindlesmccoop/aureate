PREFIX = /usr/local
	
all:
	cc aureate.c -lcurl -ltar -o aureate

verbose:
	cc aureate.c -Wall -Werror -fsanitize=undefined,address -lcurl -ltar -o aureate

install:
	install -Dm755 ./aureate ${PREFIX}/bin/aureate
PREFIX = /usr/local
	
all:
	cc aureate.c -lcurl -larchive -o aureate
	rm -rf ~/.cache

verbose:
	cc aureate.c -Wall -Werror -fsanitize=undefined,address -lcurl -larchive -o aureate

install:
	install -Dm755 ./aureate ${PREFIX}/bin/aureate
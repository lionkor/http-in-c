CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=c99 -ggdb -Werror=return-type

server: server.c
	${CC} ${CFLAGS} $^ -o $@

.PHONY: clean

clean:
	rm -f server

CC=clang
CFLAGS=-Wall -Wextra -pedantic -std=gnu99 -ggdb -Werror=return-type

server: server.c string_ops.h fs.h
	${CC} ${CFLAGS} $< -o $@

.PHONY: clean

clean:
	rm -f server

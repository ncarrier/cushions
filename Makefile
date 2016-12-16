CFLAGS=-g3 -O0 -Wall -Wextra -Werror -Wno-unused-parameter \
       -Iinclude/cushion

custom_stream:example/custom_stream.o

fopen-override.so:fopen-override/fopen-override.c
	$(CC) $^ -fPIC -shared -o $@

libcushion_src := $(wildcard src/*.c)
libcushion.so:$(libcushion_src)
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -ldl

clean:
	rm -f custom_libc_stream

.PHONY: clean

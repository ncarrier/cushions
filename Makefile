CFLAGS=-g3 -O0 -Wall -Wextra -Werror -Wno-unused-parameter

custom_stream:example/custom_stream.c
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -f custom_libc_stream

.PHONY: clean

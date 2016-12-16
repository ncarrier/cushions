CFLAGS=-g3 -O0 -Wall -Wextra -Werror -Wno-unused-parameter \
       -Iinclude/cushion

all:example/cp example/cp_no_wrap

example/cp_no_wrap:example/cp.o
	$(CC) $^ -o $@

example/cp:example/cp.o libcushion.so
	$(CC) $^ -Wl,--wrap=fopen -o $@

example/wrap/wrap_malloc:example/wrap/wrap_malloc.c example/wrap/main.c
	$(CC) -Wl,--wrap=malloc $^ -o $@

example/custom_stream:example/custom_stream.o
example/variadic_macro:example/variadic_macro.o

libcushion_src := $(wildcard src/*.c)
libcushion.so:$(libcushion_src)
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -ldl

clean:
	rm -f example/custom_stream.o \
			libcushion.so \
			example/custom_stream \
			example/custom_stream.o \
			example/variadic_macro \
			example/variadic_macro.o \
			example/cp \
			example/cp.o \
			example/cp_no_wrap \
			example/cp_no_wrap.o

.PHONY: clean all

CFLAGS := -g3 -O0 -Wall -Wextra -Werror -Wno-unused-parameter \
       -Iinclude/cushion
libcushion_src := $(wildcard src/*.c)

all:cp cp_no_wrap libcushion_override.so

cp_no_wrap:example/cp.o
	$(CC) $^ -o $@

cp:example/cp.o libcushion.so
	$(CC) $^ -Wl,--wrap=fopen -o $@

wrap_malloc:example/wrap/wrap_malloc.c example/wrap/main.c
	$(CC) -Wl,--wrap=malloc $^ -o $@

custom_stream:example/custom_stream.o
variadic_macro:example/variadic_macro.o

libcushion.so:$(libcushion_src)
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -ldl

clean:
	rm -f example/custom_stream.o \
			libcushion.so \
			custom_stream \
			example/custom_stream.o \
			variadic_macro \
			example/variadic_macro.o \
			cp \
			example/cp.o \
			cp_no_wrap \
			example/cp_no_wrap.o

.PHONY: clean all

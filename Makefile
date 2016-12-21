CFLAGS := -g3 -O0 -Wall -Wextra -Werror -Wno-unused-parameter \
       -Iinclude/cushion -Isrc
libcushion_src := $(wildcard src/*.c)

all:libcushion.so handlers_dir/libcushion_bzip2_handler.so

examples:cp cp_no_wrap
tests:mode_test

mode_test:tests/mode_test.o src/mode.o src/utils.o src/log.o
	$(CC) $^ -o $@

cp_no_wrap:example/cp.o
	$(CC) $^ -o $@

cp:example/cp.o libcushion.so
	$(CC) $^ -Wl,--wrap=fopen -o $@

wrap_malloc:example/wrap/wrap_malloc.c example/wrap/main.c
	$(CC) -Wl,--wrap=malloc $^ -o $@

custom_stream:example/custom_stream.o
variadic_macro:example/variadic_macro.o
bzip2_expand:example/bzip2/expand.c
	$(CC) $^ -o $@ $(CFLAGS) -lbz2

libcushion.so:$(libcushion_src)
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -ldl

handlers_dir/libcushion_bzip2_handler.so:handlers/bzip2_handler.c libcushion.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -L. -lbz2

check:mode_test handlers_dir/libcushion_bzip2_handler.so cp
	./mode_test
	./tests/tests.sh
	@echo "*** All test passed"

clean:
	rm -f example/custom_stream.o \
			libcushion.so \
			handlers_dir/libcushion_bzip2_handler.so \
			custom_stream \
			example/custom_stream.o \
			variadic_macro \
			bzip2_expand \
			example/variadic_macro.o \
			cp \
			example/cp.o \
			cp_no_wrap \
			example/cp_no_wrap.o \
			src/mode.o \
			src/utils.o \
			src/log.o \
			tests/mode_test.o \
			mode_test

.PHONY: clean all examples check

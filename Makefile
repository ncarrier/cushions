CFLAGS := -g3 -O0 -Wall -Wextra -Werror -Wno-unused-parameter \
       -Iinclude/cushions -Isrc
libcushions_src := $(wildcard src/*.c)

all:libcushions.so handlers_dir/libcushions_bzip2_handler.so

examples:cp cp_no_wrap
tests:mode_test

mode_test:tests/mode_test.o src/mode.o src/utils.o src/log.o
	$(CC) $^ -o $@

params_test:tests/params_test.o libcushions.so
	$(CC) $^ -o $@

cp_no_wrap:example/cp.o
	$(CC) $^ -o $@

cp:example/cp.o libcushions.so
	$(CC) $^ -Wl,--wrap=fopen -o $@

wrap_malloc:example/wrap/wrap_malloc.c example/wrap/main.c
	$(CC) -Wl,--wrap=malloc $^ -o $@

custom_stream:example/custom_stream.o
variadic_macro:example/variadic_macro.o
bzip2_expand:example/bzip2/expand.c
	$(CC) $^ -o $@ $(CFLAGS) -lbz2

libcushions.so:$(libcushions_src)
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -ldl

handlers_dir/libcushions_bzip2_handler.so:handlers/bzip2_handler.c libcushions.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -L. -lbz2

check:mode_test params_test handlers_dir/libcushions_bzip2_handler.so cp
	./mode_test
	./params_test
	./tests/tests.sh
	@echo "*** All test passed"

clean:
	rm -f example/custom_stream.o \
			libcushions.so \
			handlers_dir/libcushions_bzip2_handler.so \
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
			tests/params_test.o \
			mode_test \
			params_test

.PHONY: clean all examples check

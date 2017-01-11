# location of this Makefile
here := $(dir $(lastword $(MAKEFILE_LIST)))
VPATH := $(here)

# TODO add debug / release handling
CFLAGS := -g3 -O0

CFLAGS += \
	-I$(here)include/cushions \
	-I$(here)src

CFLAGS += \
	-Wall \
	-Wextra \
	-Wformat=2 \
	-Wunused-variable \
	-Wold-style-definition \
	-Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wpointer-arith \
	-Wformat-signedness

libcushions_src := $(wildcard $(here)src/*.c)
handlers := $(foreach h,curl bzip2,handlers_dir/libcushions_$(h)_handler.so)

all:libcushions.so $(handlers)

examples:cp cp_no_wrap
tests:mode_test

# used for cleanup and tree structure
obj := \
	example/cp.o \
	example/cp_no_wrap.o \
	example/custom_stream.o \
	example/variadic_macro.o \
	src/dict.o \
	src/log.o \
	src/mode.o \
	src/utils.o \
	tests/mode_test.o \
	tests/params_test.o

tree_structure := $(sort $(foreach s,$(obj) $(handlers),${CURDIR}/$(dir $(s))))

$(obj) $(handlers): | $(tree_structure)

$(tree_structure):
	mkdir -p $@

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

curl_fopen:example/curl_fopen.c
	$(CC) $^ -o $@ $(CFLAGS) `curl-config --cflags` `curl-config --libs`

custom_stream:example/custom_stream.o
variadic_macro:example/variadic_macro.o
bzip2_expand:example/bzip2/expand.c
	$(CC) $^ -o $@ $(CFLAGS) -lbz2

libcushions.so:$(libcushions_src)
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -ldl

handlers_dir/libcushions_bzip2_handler.so:handlers/bzip2_handler.c libcushions.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -L. -lbz2

handlers_dir/libcushions_curl_handler.so:handlers/curl_handler.c libcushions.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -L. \
			`curl-config --cflags` `curl-config --libs`

check:mode_test params_test $(handlers) cp
	CUSHIONS_LOG_LEVEL=3 LD_LIBRARY_PATH=. ./mode_test
	CUSHIONS_LOG_LEVEL=3 LD_LIBRARY_PATH=. ./params_test
	CUSHIONS_LOG_LEVEL=3 LD_LIBRARY_PATH=. $(here)tests/tests.sh
	@echo "*** All test passed"

clean:
	rm -f \
			$(obj) \
			$(handlers)
			libcushions.so \
			custom_stream \
			variadic_macro \
			bzip2_expand \
			cp \
			cp_no_wrap \
			mode_test \
			params_test

.PHONY: clean all examples check

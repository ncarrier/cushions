# location of this Makefile
here := $(dir $(lastword $(MAKEFILE_LIST)))
VPATH := $(here)
ifeq ($(CC),cc)
CC := gcc
endif

# quirk for Wformat-signedness support
GCCVERSIONGTEQ5 := $(shell expr `gcc -dumpversion | cut -f1 -d.` \>= 5)

ifeq ($(DEBUG),1)
CFLAGS := -g3 -O0
else
CFLAGS := -O3
endif

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
	-Wpointer-arith

ifeq ($(CC),gcc)
ifeq "$(GCCVERSIONGTEQ5)" "1"
CFLAGS += \
	-Wformat-signedness
endif
endif

libcushions_src := $(wildcard $(here)src/*.c)
hdlr_names := curl bzip2 lzo mem
handlers := $(foreach h,$(hdlr_names),handlers_dir/libcushions_$(h)_handler.so)

all:libcushions.so $(handlers)

examples:cp cp_no_wrap curl_fopen
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
	tests/dict_test.o \
	tests/params_test.o

tree_structure := $(sort $(foreach s,$(obj) $(handlers),${CURDIR}/$(dir $(s))))

$(obj) $(handlers): | $(tree_structure)

$(tree_structure):
	mkdir -p $@

mode_test:tests/mode_test.o src/mode.o src/utils.o src/log.o
	$(CC) $^ -o $@

dict_test:tests/dict_test.o src/dict.o
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

handlers_dir/libcushions_bzip2_handler.so:handlers/bzip2_handler.c \
		libcushions.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -L. -lbz2

handlers_dir/libcushions_curl_handler.so:handlers/curl_handler.c libcushions.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -L. \
			`curl-config --cflags` `curl-config --libs`

handlers_dir/libcushions_lzo_handler.so:handlers/lzo_handler.c libcushions.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS) -L. -llzo2

handlers_dir/libcushions_mem_handler.so:handlers/mem_handler.c libcushions.so
	$(CC) $^ -fPIC -shared -o $@ $(CFLAGS)

check:mode_test params_test dict_test $(handlers) cp
	$(here)/misc/setenv.sh ./mode_test
	$(here)/misc/setenv.sh ./params_test
	$(here)/misc/setenv.sh $(here)tests/tests.sh
	./dict_test
	@echo "*** All test passed"

clean:
	rm -f \
			$(obj) \
			$(handlers) \
			dict_test \
			libcushions.so \
			custom_stream \
			variadic_macro \
			bzip2_expand \
			cp \
			curl_fopen \
			cp_no_wrap \
			mode_test \
			params_test

.PHONY: clean all examples check

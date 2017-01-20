# location of this Makefile
here := $(dir $(lastword $(MAKEFILE_LIST)))
VPATH := $(here)
ifeq ($(CC),cc)
CC := gcc
endif

# quirk for Wformat-signedness support
GCCVERSIONGTEQ5 := $(shell expr `gcc -dumpversion | cut -f1 -d.` \>= 5)

CFLAGS := \
	-I$(here)include/cushions

CFLAGS += \
	-Wall \
	-Wextra \
	-Wformat=2 \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wpointer-arith \
	-Wno-unused-parameter \
	-Wold-style-definition \
	-Wstrict-prototypes \
	-Wunused-variable

ifeq ($(DEBUG),1)
CFLAGS += -g3 -O0 -Werror
else
CFLAGS += -O3
endif

DYN_FLAGS := -fPIC -shared

ifeq ($(CC),gcc)
ifeq "$(GCCVERSIONGTEQ5)" "1"
CFLAGS += \
	-Wformat-signedness
endif
endif

lib := libcushions.so
libcushions_src := $(wildcard $(here)src/*.c)
tests := dict_test mode_test params_test
examples := bzip2_expand cpw cp curl_fopen custom_stream variadic_macro \
	wrap_malloc

# build infos for handlers
handler_pattern := handlers_dir/libcushions_%_handler.so
handler_deps := handlers/%_handler.c $(lib)
hdlr_names := bzip2 curl lzo mem
handlers := $(foreach h,$(hdlr_names),$(subst %,$(h),$(handler_pattern)))
bzip2_extra_flags := -lbz2
curl_extra_flags := $(shell curl-config --cflags --libs)
lzo_extra_flags := -llzo2
mem_extra_flags :=

world := $(handlers) $(tests) $(examples)

all:$(lib) $(handlers)

world:$(world)
examples:$(examples)
tests:$(tests)

tree_structure := $(sort $(foreach s, $(world),${CURDIR}/$(dir $(s))))

$(handlers): | $(tree_structure)

$(tree_structure):
	mkdir -p $@

# static pattern rules allow tab-completion, pattern rules don't
$(tests): %_test: tests/%_test.c $(lib)
	$(CC) $(CFLAGS) -o $@ $^

custom_stream variadic_macro cp: %: example/%.c
	$(CC) $(CFLAGS) -o $@ $^

cpw:example/cp.c $(lib)
	$(CC) $(CFLAGS) -o $@ $^ -Wl,--wrap=fopen

wrap_malloc:example/wrap/wrap_malloc.c example/wrap/main.c
	$(CC) $(CFLAGS) -o $@ $^ -Wl,--wrap=malloc

curl_fopen:example/curl_fopen.c
	$(CC) $(CFLAGS) -o $@ $^ $(curl_extra_flags)

bzip2_expand:example/bzip2/expand.c
	$(CC) $(CFLAGS) -o $@ $^ $(bzip2_extra_flags)

$(lib):$(libcushions_src)
	$(CC) $(CFLAGS) -o $@ $^ $(DYN_FLAGS) -ldl

$(handlers): $(handler_pattern): $(handler_deps)
	$(CC) $(CFLAGS) -o $@ $^ $(DYN_FLAGS) $($*_extra_flags)

setenv := $(here)/misc/setenv.sh
check:$(tests) $(handlers) cp
	$(foreach t,$(tests),$(setenv) ./$(t))
	$(setenv) $(here)tests/tests.sh
	@echo "*** All test passed"

clean:
	rm -f \
			$(handlers) \
			$(lib) \
			$(examples) \
			$(tests)

.PHONY: clean all world examples tests check

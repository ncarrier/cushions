# location of this Makefile
here := $(dir $(lastword $(MAKEFILE_LIST)))
VPATH := $(here)
ifeq ($(CC),cc)
CC := gcc
endif

# quirk for Wformat-signedness support
GCCVERSIONGTEQ5 := $(shell expr `gcc -dumpversion | cut -f1 -d.` \>= 5)

CFLAGS := \
	-fvisibility=hidden \
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
hdlr_names := bzip2 curl lzo mem sock
handlers := $(foreach h,$(hdlr_names),$(subst %,$(h),$(handler_pattern)))
bzip2_extra_flags := -lbz2
curl_extra_flags := $(shell curl-config --cflags --libs)
lzo_extra_flags := -llzo2
mem_extra_flags :=
sock_extra_flags :=

world := $(handlers) $(tests) $(examples)

all:$(lib) $(handlers)

# generate the world.d makefile, containing dependencies on the headers, this
# is done by making the world target with a fake compiler and some bash extra
# foo too
ifneq ($(notdir $(CC)), cc_wrapper.sh)
world.d:$(shell find $(here) -name '*.h' -o -name '*.c')
	@echo Generating header dependencies handling Makefile world.d
	@CC=$(here)misc/cc_wrapper.sh make -B -s -f $(here)Makefile world | \
		sed "s/\(.*\)-o \([^ ]*\) \(.*\)/gcc \1\3 -MM -MT \2 >> $@/g" | \
		sh
endif
-include world.d

world:$(world)
examples:$(examples)
tests:$(tests)

tree_structure := $(sort $(foreach s, $(world),${CURDIR}/$(dir $(s))))

$(handlers): | $(tree_structure)

$(tree_structure):
	mkdir -p $@

# static pattern rules allow tab-completion, pattern rules don't
$(tests): %_test: tests/%_test.c $(lib)
	$(CC) $(CFLAGS) -o $@ $< -L. -lcushions

custom_stream variadic_macro cp: %: example/%.c
	$(CC) $(CFLAGS) -o $@ $^

cpw:example/cp.c $(lib)
	$(CC) $(CFLAGS) -o $@ $< -Wl,--wrap=fopen -L. -lcushions

wrap_malloc:example/wrap/wrap_malloc.c example/wrap/main.c
	$(CC) $(CFLAGS) -o $@ $^ -Wl,--wrap=malloc

curl_fopen:example/curl_fopen.c
	$(CC) $(CFLAGS) -o $@ $^ $(curl_extra_flags)

bzip2_expand:example/bzip2/expand.c
	$(CC) $(CFLAGS) -o $@ $^ $(bzip2_extra_flags)

$(lib):$(libcushions_src)
	$(CC) $(CFLAGS) -o $@ $^ $(DYN_FLAGS) -ldl

$(handlers): $(handler_pattern): $(handler_deps)
	$(CC) $(CFLAGS) -o $@ $< $(DYN_FLAGS) $($*_extra_flags) -L. -lcushions

setenv := $(here)/misc/setenv.sh
check:$(tests) $(handlers) cpw
	$(foreach t,$(tests),$(setenv) ./$(t))
	$(foreach t,$(wildcard $(here)tests/*_test.sh), $(setenv) $(t))
	@echo "*** All test passed"

clean:
	rm -f \
			$(handlers) \
			$(lib) \
			$(examples) \
			$(tests)

.PHONY: clean all world examples tests check

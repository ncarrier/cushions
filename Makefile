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

lib := libcushions
$(lib)_src := $(wildcard $(here)src/*.c)
$(lib)_src := $($(lib)_src:$(here)%=%)
tests := dict_test mode_test params_test
examples := bzip2_expand cpw cp curl_fopen custom_stream variadic_macro \
	wrap_malloc untar

# build infos for handlers
handler_pattern := handlers_dir/$(lib)_%_handler.so
handler_src_pattern := handlers/%_handler.c
handler_deps := $(handler_src_pattern)
hdlr_names := bzip2 curl lzo mem sock tar
handlers := $(foreach h,$(hdlr_names),$(subst %,$(h),$(handler_pattern)))
bzip2_extra_flags := -lbz2
curl_extra_flags := $(shell curl-config --cflags --libs)
lzo_extra_flags := -llzo2
mem_extra_flags :=
sock_extra_flags :=
tar_extra_flags :=

$(lib).a_src := $(filter-out src/cushions_handlers.c,$($(lib)_src)) \
	$(foreach h,$(hdlr_names),$(subst %,$(h),$(handler_src_pattern)))
$(lib).a_obj := $($(lib).a_src:.c=.o)

world := $(handlers) $(tests) $(examples) $(lib).so $(lib).a

all:$(lib).so $(lib).a $(handlers)

# generate the world.d makefile, containing dependencies on the headers, this
# is done by making the world target with a fake compiler and some bash extra
# foo too
ifneq ($(notdir $(CC)), cc_wrapper.sh)
world.d:$(shell find $(here) -name '*.h' -o -name '*.c')
	@echo Generating header dependencies handling Makefile world.d
	@rm -f $@
	@AR=true CC=$(here)misc/cc_wrapper.sh \
		make -B -s -f $(here)Makefile world | \
		sed "s/\(.*\)-o \([^ ]*\) \(.*\)/gcc \1\3 -MM -MT \2 >> $@/g" | \
		sh
endif
-include world.d

world:$(world)
examples:$(examples)
tests:$(tests)

tree_structure := $(sort $(foreach s, $(world) $($(lib).a_obj),${CURDIR}/$(dir $(s))))

$(handlers) $($(lib).a_obj): | $(tree_structure)

$(tree_structure):
	mkdir -p $@

# static pattern rules allow tab-completion, pattern rules don't
$(tests): %_test: tests/%_test.c $(lib).so
	$(CC) $(CFLAGS) -o $@ $< -L. -lcushions

custom_stream variadic_macro cp: %: example/%.c
	$(CC) $(CFLAGS) -o $@ $<

untar: example/untar.c handlers/tar.c
	$(CC) $(CFLAGS) -o $@ $^ -I$(here)handlers

cpw:example/cp.c $(lib).so
	$(CC) $(CFLAGS) -o $@ $< -Wl,--wrap=fopen -L. -lcushions

wrap_malloc:example/wrap/wrap_malloc.c example/wrap/main.c
	$(CC) $(CFLAGS) -o $@ $^ -Wl,--wrap=malloc

curl_fopen:example/curl_fopen.c
	$(CC) $(CFLAGS) -o $@ $^ $(curl_extra_flags)

bzip2_expand:example/bzip2/expand.c
	$(CC) $(CFLAGS) -o $@ $^ $(bzip2_extra_flags)

$(lib).so:$($(lib)_src)
	$(CC) $(CFLAGS) -o $@ $^ $(DYN_FLAGS) -ldl

$(lib).a:$($(lib).a_obj)
	$(AR) crs $@ $^

ifneq ($(notdir $(CC)), cc_wrapper.sh)
$(handlers): $(handler_pattern): $(lib).so
endif
handlers_dir/$(lib)_tar_handler.so: handlers/tar.c
$(handlers): $(handler_pattern): $(handler_deps)
	$(CC) $(CFLAGS) -o $@ $^ $(DYN_FLAGS) $($*_extra_flags)

setenv := $(here)/misc/setenv.sh
check:$(tests) $(handlers) cpw
	$(foreach t,$(tests),$(setenv) ./$(t))
	$(foreach t,$(wildcard $(here)tests/*_test.sh), $(setenv) $(t);)
	@echo "*** All test passed"

clean:
	rm -f \
			$(handlers) \
			$(examples) \
			$(lib).so \
			$(lib).a \
			$($(lib).a_obj) \
			$(tests) \
			world.d

.PHONY: clean all world examples tests check

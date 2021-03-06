#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <cushions_handler_mode.h>

#define ST(m) STRINGIFY(m)
#define assert_null(p) assert((p) == NULL || !ST(p should be null))
#define assert_not_null(p) assert((p) != NULL || !ST(p should be null))
#define assert_str_equal(a, b) assert((a != NULL) && (b != NULL) && \
		strcmp((a), (b)) == 0)
#define masserteq(m, r) do { \
	assert((m).read == (r).read); \
	assert((m).beginning == (r).beginning); \
	assert((m).write == (r).write); \
	assert((m).truncate == (r).truncate); \
	assert((m).create == (r).create); \
	assert((m).binary == (r).binary); \
	assert((m).append == (r).append); \
	assert((m).no_cancellation == (r).no_cancellation); \
	assert((m).cloexec == (r).cloexec); \
	assert((m).mmap == (r).mmap); \
	assert((m).excl == (r).excl); \
	assert(!m.ccs == !r.ccs); \
	if (m.ccs != NULL) \
		assert(strcmp(m.ccs, r.ccs) == 0); \
} while (0)

static void cushions_handler_mode_from_stringTEST(void)
{
	struct ch_mode mode;
	struct ch_mode ref;
	const char *test_mode;

	fprintf(stderr, "%s\n", __func__);

	fprintf(stderr, "test string %s\n", test_mode = "rb+cmxe");
	assert(ch_mode_from_string(&mode, test_mode) == 0);
	ch_mode_dump(&mode);
	ref = (struct ch_mode){
		.read = 1,
		.beginning = 1,
		.write = 1,
		.truncate = 0,
		.create = 0,
		.binary = 1,
		.append = 0,
		.no_cancellation = 1,
		.cloexec = 1,
		.mmap = 1,
		.excl = 1,
		.ccs = NULL,
	};
	masserteq(mode, ref);
	assert_str_equal(mode.mode, test_mode);
	ch_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "rb+cmxe,ccs=blabla");
	assert(ch_mode_from_string(&mode, test_mode) == 0);
	ch_mode_dump(&mode);
	ref.ccs = "blabla";
	masserteq(mode, ref);
	assert_str_equal(mode.mode, test_mode);
	ch_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "a");
	assert(ch_mode_from_string(&mode, test_mode) == 0);
	ch_mode_dump(&mode);
	ref = (struct ch_mode){
		.read = 0,
		.beginning = 0,
		.write = 1,
		.truncate = 0,
		.create = 1,
		.binary = 0,
		.append = 1,
		.no_cancellation = 0,
		.cloexec = 0,
		.mmap = 0,
		.excl = 0,
		.ccs = NULL,
	};
	masserteq(mode, ref);
	assert_str_equal(mode.mode, test_mode);
	ch_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "a,ccs=");
	assert(ch_mode_from_string(&mode, test_mode) == 0);
	ch_mode_dump(&mode);
	ref.ccs = "";
	masserteq(mode, ref);
	assert_str_equal(mode.mode, test_mode);
	ch_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "ad,ccs=");
	assert(ch_mode_from_string(&mode, test_mode) != 0);

	assert(ch_mode_from_string(NULL, "a") != 0);
	assert(ch_mode_from_string(&mode, NULL) != 0);
	assert(ch_mode_from_string(&mode, "") != 0);
	assert(ch_mode_from_string(&mode, ",ccs=bla") != 0);

	fprintf(stderr, "OK\n");
}

static void cushions_handler_mode_to_stringTEST(void)
{
	struct ch_mode mode;
	struct ch_mode ref;
	char *str;

	fprintf(stderr, "%s\n", __func__);

	ref = (struct ch_mode){
		.read = 0,
		.beginning = 0,
		.write = 1,
		.truncate = 0,
		.create = 1,
		.binary = 0,
		.append = 1,
		.no_cancellation = 0,
		.cloexec = 0,
		.mmap = 0,
		.excl = 0,
		.ccs = NULL,
	};
	assert(ch_mode_to_string(&ref, &str) == 0);
	assert(ch_mode_from_string(&mode, str) == 0);
	ch_mode_dump(&mode);
	masserteq(mode, ref);
	ch_mode_cleanup(&mode);
	free(str);

	fprintf(stderr, "OK\n");
}

int main(void)
{
	cushions_handler_mode_from_stringTEST();
	cushions_handler_mode_to_stringTEST();

	return EXIT_SUCCESS;
}

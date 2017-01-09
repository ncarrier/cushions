#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <cushions_handler.h>
#include "utils.h"

#define S(m) STRINGIFY(m)
#define assert_null(p) assert((p) == NULL || !S(p should be null))
#define assert_not_null(p) assert((p) != NULL || !S(p should be null))
#define assert_str_equal(a, b) assert((a != NULL) && (b != NULL) && \
		strcmp((a), (b)) == 0)
#define masserteq(m, r) do { \
	assert((m).read == (r).read); \
	assert((m).beginning == (r).beginning); \
	assert((m).end == (r).end); \
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
	struct cushions_handler_mode mode;
	struct cushions_handler_mode ref;
	const char *test_mode;

	fprintf(stderr, "%s\n", __func__);

	fprintf(stderr, "test string %s\n", test_mode = "rb+cmxe");
	assert(cushions_handler_mode_from_string(&mode, test_mode) == 0);
	cushions_handler_mode_dump(&mode);
	ref = (struct cushions_handler_mode){
		.read = 1,
		.beginning = 1,
		.end = 0,
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
	cushions_handler_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "rb+cmxe,ccs=blabla");
	assert(cushions_handler_mode_from_string(&mode, test_mode) == 0);
	cushions_handler_mode_dump(&mode);
	ref.ccs = "blabla";
	masserteq(mode, ref);
	assert_str_equal(mode.mode, test_mode);
	cushions_handler_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "a");
	assert(cushions_handler_mode_from_string(&mode, test_mode) == 0);
	cushions_handler_mode_dump(&mode);
	ref = (struct cushions_handler_mode){
		.read = 0,
		.beginning = 0,
		.end = 1,
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
	cushions_handler_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "a,ccs=");
	assert(cushions_handler_mode_from_string(&mode, test_mode) == 0);
	cushions_handler_mode_dump(&mode);
	ref.ccs = "";
	masserteq(mode, ref);
	assert_str_equal(mode.mode, test_mode);
	cushions_handler_mode_cleanup(&mode);

	fprintf(stderr, "test string %s\n", test_mode = "ad,ccs=");
	assert(cushions_handler_mode_from_string(&mode, test_mode) != 0);

	assert(cushions_handler_mode_from_string(NULL, "a") != 0);
	assert(cushions_handler_mode_from_string(&mode, NULL) != 0);
	assert(cushions_handler_mode_from_string(&mode, "") != 0);
	assert(cushions_handler_mode_from_string(&mode, ",ccs=bla") != 0);

	fprintf(stderr, "OK\n");
}

static void cushions_handler_mode_to_stringTEST(void)
{
	struct cushions_handler_mode mode;
	struct cushions_handler_mode ref;
	char *str;

	fprintf(stderr, "%s\n", __func__);

	ref = (struct cushions_handler_mode){
		.read = 0,
		.beginning = 0,
		.end = 1,
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
	assert(cushions_handler_mode_to_string(&ref, &str) == 0);
	assert(cushions_handler_mode_from_string(&mode, str) == 0);
	cushions_handler_mode_dump(&mode);
	masserteq(mode, ref);
	free(str);

	fprintf(stderr, "OK\n");
}

int main(void)
{
	cushions_handler_mode_from_stringTEST();
	cushions_handler_mode_to_stringTEST();

	return EXIT_SUCCESS;
}

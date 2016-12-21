#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <envz.h>

#include <cushion_handler.h>

#define assert_str_equal(s1, s2) assert(strcmp((s1), (s2)) == 0)
#define assert_null(p) assert((p) == NULL)

int main(void)
{
	const char *input;
	char *path;
	char *envz;
	size_t len;

	input = "/tata/tutu/toto?plop=wizz;foo=bar;baz=";
	fprintf(stderr, "input is \"%s\"\n", input);
	assert(cushion_handler_break_params(input, &path, &envz, &len) == 0);
	assert_str_equal(path, "/tata/tutu/toto");
	assert_str_equal(envz_get(envz, len, "plop"), "wizz");
	assert_str_equal(envz_get(envz, len, "foo"), "bar");
	assert_str_equal(envz_get(envz, len, "baz"), "");
	assert_null(envz_get(envz, len, "tadaaa"));
	free(path);
	free(envz);

	input = "/tata/tutu/toto?";
	fprintf(stderr, "input is \"%s\"\n", input);
	assert(cushion_handler_break_params(input, &path, &envz, &len) == 0);
	assert_str_equal(path, "/tata/tutu/toto");
	assert_null(envz);
	assert(len == 0);
	free(path);

	input = "";
	fprintf(stderr, "input is \"%s\"\n", input);
	assert(cushion_handler_break_params(input, &path, &envz, &len) == 0);
	assert_null(envz);
	assert_null(path);
	assert(len == 0);

	input = "?plop=wizz;foo=bar;baz";
	fprintf(stderr, "input is \"%s\"\n", input);
	assert(cushion_handler_break_params(input, &path, &envz, &len) == 0);
	assert_str_equal(path, "");
	assert_str_equal(envz_get(envz, len, "plop"), "wizz");
	assert_str_equal(envz_get(envz, len, "foo"), "bar");
	assert(envz_get(envz, len, "baz") == NULL);
	assert_null(envz_get(envz, len, "tadaaa"));
	free(path);
	free(envz);

	return EXIT_SUCCESS;
}

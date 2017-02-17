#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "cushions_handler_dict.h"

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof(*(a)))

static const struct ch_dict_node dict = CH_DICT(F(T(P(_))),
                                                S(C(P(_)),
                                                  M(B(S(_),
                                                        _)),
                                                F(T(P(_)))),
                                                H(T(T(P(S(_),
                                                        _)))),
                                                T(F(T(P(_)))));

static void print_cb(const char *string, void *data)
{
	printf("%s\n", string);
}

struct test_case {
	const char *string;
	bool expected_result;
};

static const struct test_case tests[] = {
		{ .string = "blabla", .expected_result = false},
		{ .string = "ftp",    .expected_result = true},
		{ .string = "http",   .expected_result = true},
		{ .string = "https",  .expected_result = true},
		{ .string = "scp",    .expected_result = true},
		{ .string = "sftp",   .expected_result = true},
		{ .string = "smb",    .expected_result = true},
		{ .string = "smbs",   .expected_result = true},
		{ .string = "snb",    .expected_result = false},
		{ .string = "tftp",   .expected_result = true},
		{ .string = "uftp",   .expected_result = false},
		{ .string = "tfup",   .expected_result = false},
		{ .string = "tftr",   .expected_result = false},
		{ .string = "tgtp",   .expected_result = false},
		{ .string = "",       .expected_result = false},
		{ .string = NULL,     .expected_result = false},
};

int main(void)
{
	const struct test_case *t;
	bool result;

	printf("strings in dictionary:\n");
	ch_dict_foreach_word(&dict, print_cb, NULL);

	for (t = tests; (size_t)(t - tests) < ARRAY_SIZE(tests); t++) {
		result = dict_contains(&dict, t->string);
		printf("test string: '%s', matches: %s\n",
				t->string != NULL ? t->string : "(NULL)",
				result ? "yes" : "no");
		assert(t->expected_result == result);
	}

	return EXIT_SUCCESS;
}

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#define STRINGIFY_HELPER(s) #s
#define STRINGIFY(s) STRINGIFY_HELPER(s)

struct dict_node {
	char c;
	struct dict_node *next;
};

#define EOT 0x04

#define DICT_TERMINAL(ch) { .c = (ch), .next = NULL}
#define DICT_GUARD DICT_TERMINAL(EOT)
#define _ DICT_TERMINAL('\0')
#define DICT_NODE(p, ...) { .c = p, .next = (struct dict_node[]){__VA_ARGS__, \
	DICT_GUARD}}
#define DICT(...) DICT_NODE(' ', __VA_ARGS__)
#define B(...) DICT_NODE('b', __VA_ARGS__)
#define C(...) DICT_NODE('c', __VA_ARGS__)
#define F(...) DICT_NODE('f', __VA_ARGS__)
#define H(...) DICT_NODE('h', __VA_ARGS__)
#define M(...) DICT_NODE('m', __VA_ARGS__)
#define P(...) DICT_NODE('p', __VA_ARGS__)
#define S(...) DICT_NODE('s', __VA_ARGS__)
#define T(...) DICT_NODE('t', __VA_ARGS__)

static struct dict_node dict = DICT(F(T(P(_))),
                                    S(C(P(_)),
                                      M(B(S(_),
                                            _)),
                                    F(T(P(_)))),
                                    H(T(T(P(S(_),
                                            _)))),
                                    T(F(T(P(_)))));

typedef void (*dict_cb)(const char *string, void *data);

static void dict_foreach_word_rec(const struct dict_node *node, char *buf,
		char *c, dict_cb cb, void *data)
{
	const struct dict_node *cur;

	*c = node->c;
	if (node->c == '\0')
		return cb(buf, data);
	if (node->next == NULL)
		return;

	for (cur = node->next; cur->c != EOT; cur++)
		dict_foreach_word_rec(cur, buf, c + 1, cb, data);
}

static void dict_foreach_word(const struct dict_node *dict, dict_cb cb,
		void *data)
{
	char buf[0x400];

	dict_foreach_word_rec(dict, buf, buf, cb, data);
}

static bool dict_contains_rec(const char *str, const struct dict_node *node)
{
	const struct dict_node *cur;

	if (*str != node->c)
		return false;

	if (node->c == '\0')
		return true;

	for (cur = node->next; cur->c != EOT; cur++)
		if (dict_contains_rec(str + 1, cur))
			return true;

	return false;
}

static bool dict_contains(const struct dict_node *dict, const char *str)
{
	const struct dict_node *cur;

	if (str == NULL || dict == NULL)
		return false;

	for (cur = dict->next; cur->c != EOT; cur++)
		if (dict_contains_rec(str, cur))
			return true;

	return false;
}

static void print_cb(const char *string, void *data)
{
	printf("%s\n", string);
}

int main(void)
{
	const char *strings[] = {
			"smb",
			"snb",
			"ftp",
			"https",
			"http",
			"scp",
			"smbs",
			"smb",
			"sftp",
			"tftp",
			"uftp",
			"tgtp",
			"tfup",
			"tftr",
			"blabla",
			"",
			NULL
	};
	const char **string;

	printf("strings in dictionnary:\n");
	dict_foreach_word(&dict, print_cb, NULL);

	for (string = strings; *string != NULL; string++)
		printf("test string: '%s', matches: %s\n", *string,
				dict_contains(&dict, *string) ? "yes" : "no");
	printf("test string: (NULL), matches: %s\n",
			dict_contains(&dict, NULL) ? "yes" : "no");

	return EXIT_SUCCESS;
}

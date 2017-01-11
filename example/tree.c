#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

struct node {
	char c;
	struct node *next;
};

#define EOT 0x04

#define TREE_TERMINAL(ch) { .c = (ch), .next = NULL}
#define TREE_GUARD TREE_TERMINAL(EOT)
#define L TREE_TERMINAL('\0')
#define N(ch, ...) { .c = ch, .next = (struct node[]){__VA_ARGS__, TREE_GUARD}}

/*
 * The following state machine contains the strings :
 *   "ftp"
 *   "https"
 *   "http"
 *   "scp"
 *   "smbs"
 *   "smb"
 *   "sftp"
 *   "tftp"
 */
static struct node tree =
  N(' ', N('f', N('t', N('p', L))), /* ftp */
         N('s', N('c', N('p', L)), /* scp */
                N('m', N('b', N('s', L), /* smbs */
                              L)), /* smb */
         N('f', N('t', N('p', L)))), /* ftp */
         N('h', N('t', N('t', N('p', N('s', L), /* https */
                                     L)))), /* http */
         N('t', N('f', N('t', N('p', L))))); /* tftp */

typedef void (*cb)(const char *string, void *data);

static void foreach_leaves_rec(const struct node *node, char *buf, char *c,
		cb cb, void *data)
{
	const struct node *cur;

	*c = node->c;
	if (node->c == '\0')
		return cb(buf, data);
	if (node->next == NULL)
		return;

	for (cur = node->next; cur->c != EOT; cur++)
		foreach_leaves_rec(cur, buf, c + 1, cb, data);
}

static void foreach_leaves(const struct node *tree, cb cb, void *data)
{
	char buf[0x400];

	foreach_leaves_rec(tree, buf, buf, cb, data);
}

static bool match_rec(const char *str, const struct node *node)
{
	const struct node *cur;

	if (*str != node->c)
		return false;

	if (node->c == '\0')
		return true;

	for (cur = node->next; cur->c != EOT; cur++)
		if (match_rec(str + 1, cur))
			return true;

	return false;
}

static bool match(const char *str, const struct node *tree)
{
	const struct node *cur;

	if (str == NULL || tree == NULL)
		return false;

	for (cur = tree->next; cur->c != EOT; cur++)
		if (match_rec(str, cur))
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

	printf("strings in tree:\n");
	foreach_leaves(&tree, print_cb, NULL);

	for (string = strings; *string != NULL; string++)
		printf("test string: '%s', matches: %s\n", *string,
				match(*string, &tree) ? "yes" : "no");
	printf("test string: (NULL), matches: %s\n",
			match(NULL, &tree) ? "yes" : "no");

	return EXIT_SUCCESS;
}

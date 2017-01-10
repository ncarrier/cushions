#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

struct node {
	struct node *next;
	char c;
};

#define EOT 0x04

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
static struct node tree = {
		.c = ' ',
		.next = (struct node[])
{
	{
		.c = 'f',
		.next = (struct node[]) {
			{
				.c = 't',
				.next = (struct node[] ) {
					{
						.c = 'p',
						.next = (struct node[] ) {
							{
								.c = '\0', /* ftp */
								.next = NULL,
							},
							{
								.c = EOT
							}
						}
					},
					{
						.c = EOT
					}
				}
			},
			{
				.c = EOT
			}
		}
	},
	{
		.c = 's',
		.next = (struct node[] ) {
			{
				.c = 'c',
				.next = (struct node[] ) {
					{
						.c = 'p',
						.next = (struct node[] ) {
							{
								.c = '\0', /* scp */
								.next = NULL,
							},
							{
								.c = EOT
							}
						}
					},
					{
						.c = EOT
					}
				}
			},
			{
				.c = 'm',
				.next = (struct node[] ) {
					{
						.c = 'b',
						.next = (struct node[] ) {
							{
								.c = 's',
								.next = (struct node[] ) {
									{
										.c = '\0', /* smbs */
										.next = NULL,
									},
									{
										.c = EOT
									}
								}
							},
							{
								.c = '\0', /* smb */
								.next = NULL,
							},
							{
								.c = EOT
							}
						}
					},
					{
						.c = EOT
					}
				}
			},
			{
				.c = 'f',
				.next = (struct node[] ) {
					{
						.c = 't',
						.next = (struct node[] ) {
							{
								.c = 'p',
								.next = (struct node[] ) {
									{
										.c = '\0', /* sftp */
										.next = NULL,
									},
									{
										.c = EOT
									}
								}
							},
							{
								.c = EOT
							}
						}
					},
					{
						.c = EOT
					}
				}
			},
			{
				.c = EOT
			}
		}
	},
	{
		.c = 'h',
		.next = (struct node[] ) {
			{
				.c = 't',
				.next = (struct node[] ) {
					{
						.c = 't',
						.next = (struct node[] ) {
							{
								.c = 'p',
								.next = (struct node[] ) {
									{
										.c = 's',
										.next = (struct node[] ) {
											{
												.c = '\0', /* https */
												.next = NULL,
											},
											{
												.c = EOT
											}
										}
									},
									{
										.c = '\0', /* http */
										.next = NULL,
									},
									{
										.c = EOT
									}
								}
							},
							{
								.c = EOT
							}
						}
					},
					{
						.c = EOT
					}
				}
			},
			{
				.c = EOT
			}
		}
	},
	{
		.c = 't',
		.next = (struct node[]) {
			{
				.c = 'f',
				.next = (struct node[] ) {
					{
						.c = 't',
						.next = (struct node[] ) {
							{
								.c = 'p',
								.next = (struct node[] ) {
									{
										.c = '\0', /* tftp */
										.next = NULL,
									},
									{
										.c = EOT
									}
								}
							},
							{
								.c = EOT
							}
						}
					},
					{
						.c = EOT
					}
				}
			},
			{
				.c = EOT
			}
		}
	},
	{
		.c = EOT
	}
}
};

typedef void (*cb)(const char *string, void *data);

static void foreach_states_rec(const struct node *node, char *buf, char *c, cb cb,
		void *data)
{
	const struct node *cur;

	*c = node->c;
	if (node->c == '\0')
		return cb(buf, data);
	if (node->next == NULL)
		return;

	for (cur = node->next; cur->c != EOT; cur++)
		foreach_states_rec(cur, buf, c + 1, cb, data);
}

static void foreach_states(const struct node *tree, cb cb, void *data)
{
	char buf[0x400];

	foreach_states_rec(tree, buf, buf, cb, data);
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
	foreach_states(&tree, print_cb, NULL);

	for (string = strings; *string != NULL; string++)
		printf("test string: '%s', matches: %s\n", *string,
				match(*string, &tree) ? "yes" : "no");
	printf("test string: (NULL), matches: %s\n",
			match(NULL, &tree) ? "yes" : "no");

	return EXIT_SUCCESS;
}

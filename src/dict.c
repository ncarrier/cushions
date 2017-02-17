#include <stdlib.h>
#include <stdio.h>

#include "cushions_handler_dict.h"

static void dict_foreach_word_rec(const struct ch_dict_node *node, char *buf,
		char *c, ch_dict_cb cb, void *data)
{
	const struct ch_dict_node *cur;
	bool out_of_room;

	out_of_room = (c - buf) >= (CH_DICT_FOREACH_MAX_WORD_LEN - 1);
	if (out_of_room) {
		/* out of room in the buffer */
		*c = '\0';
	}
	*c = node->c;
	if (node->c == '\0')
		return cb(buf, data);
	if (node->next == NULL)
		return;

	for (cur = node->next; cur->c != CH_DICT_EOT; cur++)
		dict_foreach_word_rec(cur, buf, c + !out_of_room, cb, data);
}

static bool dict_contains_rec(const char *str, const struct ch_dict_node *node)
{
	const struct ch_dict_node *cur;

	if (*str != node->c)
		return false;

	if (node->c == '\0')
		return true;

	for (cur = node->next; cur->c != CH_DICT_EOT; cur++)
		if (dict_contains_rec(str + 1, cur))
			return true;

	return false;
}

void ch_dict_foreach_word(const struct ch_dict_node *dict, ch_dict_cb cb,
		void *data)
{
	const struct ch_dict_node *cur;

	char buf[CH_DICT_FOREACH_MAX_WORD_LEN];

	for (cur = dict->next; cur->c != CH_DICT_EOT; cur++)
		dict_foreach_word_rec(cur, buf, buf, cb, data);
}

bool dict_contains(const struct ch_dict_node *dict, const char *str)
{
	const struct ch_dict_node *cur;

	if (str == NULL || dict == NULL)
		return false;

	for (cur = dict->next; cur->c != CH_DICT_EOT; cur++)
		if (dict_contains_rec(str, cur))
			return true;

	return false;
}

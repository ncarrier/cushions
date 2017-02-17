/*
 * one should not need to include this header directly, include
 * cushions_handler.h instead
 */
#ifndef CUSHIONS_HANDLERS_DICT_H_
#define CUSHIONS_HANDLERS_DICT_H_

#include <stdbool.h>
#include <stdarg.h>

#include <cushions_common.h>

struct ch_dict_node {
	char c;
	struct ch_dict_node *next;
};

#define CH_DICT_EOT 0x04

/*
 * word length above which, the string will be truncated when a dict_cb is
 * called during a dict_foreach_word()
 */
#define CH_DICT_FOREACH_MAX_WORD_LEN 0x400

#define CH_DICT_TERMINAL(ch) { .c = (ch), .next = NULL}
#define CH_DICT_GUARD CH_DICT_TERMINAL(CH_DICT_EOT)
#define CH_DICT_NODE(p, ...) { .c = p, .next = (struct ch_dict_node[]){ \
	__VA_ARGS__, CH_DICT_GUARD} }

#define CH_DICT(...) CH_DICT_NODE('\0', __VA_ARGS__)
#define _ CH_DICT_TERMINAL('\0')
#define A(...) CH_DICT_NODE('a', __VA_ARGS__)
#define B(...) CH_DICT_NODE('b', __VA_ARGS__)
#define C(...) CH_DICT_NODE('c', __VA_ARGS__)
#define D(...) CH_DICT_NODE('d', __VA_ARGS__)
#define E(...) CH_DICT_NODE('e', __VA_ARGS__)
#define F(...) CH_DICT_NODE('f', __VA_ARGS__)
#define G(...) CH_DICT_NODE('g', __VA_ARGS__)
#define H(...) CH_DICT_NODE('h', __VA_ARGS__)
#define I(...) CH_DICT_NODE('i', __VA_ARGS__)
#define J(...) CH_DICT_NODE('j', __VA_ARGS__)
#define K(...) CH_DICT_NODE('k', __VA_ARGS__)
#define L(...) CH_DICT_NODE('l', __VA_ARGS__)
#define M(...) CH_DICT_NODE('m', __VA_ARGS__)
#define N(...) CH_DICT_NODE('n', __VA_ARGS__)
#define O(...) CH_DICT_NODE('o', __VA_ARGS__)
#define P(...) CH_DICT_NODE('p', __VA_ARGS__)
#define Q(...) CH_DICT_NODE('q', __VA_ARGS__)
#define R(...) CH_DICT_NODE('r', __VA_ARGS__)
#define S(...) CH_DICT_NODE('s', __VA_ARGS__)
#define T(...) CH_DICT_NODE('t', __VA_ARGS__)
#define U(...) CH_DICT_NODE('u', __VA_ARGS__)
#define V(...) CH_DICT_NODE('v', __VA_ARGS__)
#define W(...) CH_DICT_NODE('w', __VA_ARGS__)
#define X(...) CH_DICT_NODE('x', __VA_ARGS__)
#define Y(...) CH_DICT_NODE('y', __VA_ARGS__)
#define Z(...) CH_DICT_NODE('z', __VA_ARGS__)

typedef void (*ch_dict_cb)(const char *string, void *data);

CUSHIONS_API void ch_dict_foreach_word(const struct ch_dict_node *dict,
		ch_dict_cb cb, void *data);
CUSHIONS_API bool dict_contains(const struct ch_dict_node *dict,
		const char *str);

#endif /* CUSHIONS_HANDLERS_DICT_H_ */

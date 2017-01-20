#ifndef SRC_DICT_H_
#define SRC_DICT_H_

#include <stdbool.h>
#include <stdarg.h>

struct dict_node {
	char c;
	struct dict_node *next;
};

#define DICT_EOT 0x04

/*
 * word length above which, the string will be truncated when a dict_cb is
 * called during a dict_foreach_word()
 */
#define DICT_FOREACH_MAX_WORD_LEN 0x400

#define DICT_TERMINAL(ch) { .c = (ch), .next = NULL}
#define DICT_GUARD DICT_TERMINAL(DICT_EOT)
#define DICT_NODE(p, ...) { .c = p, .next = (struct dict_node[]){__VA_ARGS__, \
	DICT_GUARD}}

#define DICT(...) DICT_NODE('\0', __VA_ARGS__)
#define _ DICT_TERMINAL('\0')
#define A(...) DICT_NODE('a', __VA_ARGS__)
#define B(...) DICT_NODE('b', __VA_ARGS__)
#define C(...) DICT_NODE('c', __VA_ARGS__)
#define D(...) DICT_NODE('d', __VA_ARGS__)
#define E(...) DICT_NODE('e', __VA_ARGS__)
#define F(...) DICT_NODE('f', __VA_ARGS__)
#define G(...) DICT_NODE('g', __VA_ARGS__)
#define H(...) DICT_NODE('h', __VA_ARGS__)
#define I(...) DICT_NODE('i', __VA_ARGS__)
#define J(...) DICT_NODE('j', __VA_ARGS__)
#define K(...) DICT_NODE('k', __VA_ARGS__)
#define L(...) DICT_NODE('l', __VA_ARGS__)
#define M(...) DICT_NODE('m', __VA_ARGS__)
#define N(...) DICT_NODE('n', __VA_ARGS__)
#define O(...) DICT_NODE('o', __VA_ARGS__)
#define P(...) DICT_NODE('p', __VA_ARGS__)
#define Q(...) DICT_NODE('q', __VA_ARGS__)
#define R(...) DICT_NODE('r', __VA_ARGS__)
#define S(...) DICT_NODE('s', __VA_ARGS__)
#define T(...) DICT_NODE('t', __VA_ARGS__)
#define U(...) DICT_NODE('u', __VA_ARGS__)
#define V(...) DICT_NODE('v', __VA_ARGS__)
#define W(...) DICT_NODE('w', __VA_ARGS__)
#define X(...) DICT_NODE('x', __VA_ARGS__)
#define Y(...) DICT_NODE('y', __VA_ARGS__)
#define Z(...) DICT_NODE('z', __VA_ARGS__)

typedef void (*dict_cb)(const char *string, void *data);

void dict_foreach_word(const struct dict_node *dict, dict_cb cb, void *data);
bool dict_contains(const struct dict_node *dict, const char *str);

#endif /* SRC_DICT_H_ */

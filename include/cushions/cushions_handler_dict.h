/**
 * @file cushions_handler_dict.h
 * @brief Simple and lightweight string dictonary based on a macro-based n-ary
 * tree structure implementation.
 *
 * User should declare a dictionary this way:
 *
 *        struct ch_dict_node dict = CH_DICT(F(T(P(_))),
 *                                           S(C(P(_)),
 *                                             M(B(S(_),
 *                                                 _)),
 *                                           F(T(P(_)))),
 *                                           H(T(T(P(S(_),
 *                                                   _)))),
 *                                           T(F(T(P(_)))));
 *
 * Which will contain the following words list, order from top to bottom:
 * "ftp", "scp", "smbs", "smb", "ftp", "https", "http", "tftp".
 * Then the dict_contains() function can be used to test if a string is
 * contained by the dictionnary.
 *
 * Macros for building dictionaries for all the 26 lower-case Latin letters are
 * provided.
 *
 * One should not need to include this header directly, include
 * cushions_handler.h instead.
 */
#ifndef CUSHIONS_HANDLERS_DICT_H_
#define CUSHIONS_HANDLERS_DICT_H_

#include <stdbool.h>
#include <stdarg.h>

#include <cushions_common.h>

/**
 * @struct ch_dict_node
 * @brief Node for building a dictionnary tree structure.
 */
struct ch_dict_node {
	/**
	 * Character contained by this node, can be '\0' for end of string, or
	 * CH_DICT_EOT to mark the last child.
	 */
	char c;
	/**
	 * Array of this node's children, terminated by a CH_DICT_GUARD.
	 */
	struct ch_dict_node *next;
};

/**
 * @def CH_DICT_EOT
 * @brief Character value of a node marking the end of a children list.
 */
#define CH_DICT_EOT 0x04

/**
 * @def CH_DICT_FOREACH_MAX_WORD_LEN
 * @brief Word length above which, the string will be truncated when a dict_cb
 * is called during a dict_foreach_word().
 */
#define CH_DICT_FOREACH_MAX_WORD_LEN 0x400

/**
 * @def CH_DICT_TERMINAL
 * @brief Builds a node with no children, for a given character value.
 */
#define CH_DICT_TERMINAL(ch) { .c = (ch), .next = NULL}

/**
 * @def CH_DICT_GUARD
 * @brief Node marking the end of a children list.
 */
#define CH_DICT_GUARD CH_DICT_TERMINAL(CH_DICT_EOT)

/**
 * @def CH_DICT_NODE
 * @brief Builds a non terminal node, given a character and the list of its
 * children.
 * @param p Character of this node.
 * @param ... List of the node's children.
 */
/* note: p is used instead of c, because it would collide with the c member. */
#define CH_DICT_NODE(p, ...) { .c = p, .next = (struct ch_dict_node[]){ \
	__VA_ARGS__, CH_DICT_GUARD} }

/**
 * @def CH_DICT
 * @brief Builds a dictionary from its root, knowing the root's children list.
 * @param ... List of the dictionnary's root's children.
 */
#define CH_DICT(...) CH_DICT_NODE('\0', __VA_ARGS__)

/**
 * @def _
 * @brief Ends a word.
 */
#define _ CH_DICT_TERMINAL('\0')

/**
 * @def A
 * @brief Builds a node containing the letter 'a'.
 * @note All the macros B ... Z are similar to this one.
 * @param ... List of the node's children
 */
#define A(...) CH_DICT_NODE('a', __VA_ARGS__)
#define B(...) CH_DICT_NODE('b', __VA_ARGS__) /**< 'b' letter node. */
#define C(...) CH_DICT_NODE('c', __VA_ARGS__) /**< 'c' letter node. */
#define D(...) CH_DICT_NODE('d', __VA_ARGS__) /**< 'd' letter node. */
#define E(...) CH_DICT_NODE('e', __VA_ARGS__) /**< 'e' letter node. */
#define F(...) CH_DICT_NODE('f', __VA_ARGS__) /**< 'f' letter node. */
#define G(...) CH_DICT_NODE('g', __VA_ARGS__) /**< 'g' letter node. */
#define H(...) CH_DICT_NODE('h', __VA_ARGS__) /**< 'h' letter node. */
#define I(...) CH_DICT_NODE('i', __VA_ARGS__) /**< 'i' letter node. */
#define J(...) CH_DICT_NODE('j', __VA_ARGS__) /**< 'j' letter node. */
#define K(...) CH_DICT_NODE('k', __VA_ARGS__) /**< 'k' letter node. */
#define L(...) CH_DICT_NODE('l', __VA_ARGS__) /**< 'l' letter node. */
#define M(...) CH_DICT_NODE('m', __VA_ARGS__) /**< 'm' letter node. */
#define N(...) CH_DICT_NODE('n', __VA_ARGS__) /**< 'n' letter node. */
#define O(...) CH_DICT_NODE('o', __VA_ARGS__) /**< 'o' letter node. */
#define P(...) CH_DICT_NODE('p', __VA_ARGS__) /**< 'p' letter node. */
#define Q(...) CH_DICT_NODE('q', __VA_ARGS__) /**< 'q' letter node. */
#define R(...) CH_DICT_NODE('r', __VA_ARGS__) /**< 'r' letter node. */
#define S(...) CH_DICT_NODE('s', __VA_ARGS__) /**< 's' letter node. */
#define T(...) CH_DICT_NODE('t', __VA_ARGS__) /**< 't' letter node. */
#define U(...) CH_DICT_NODE('u', __VA_ARGS__) /**< 'u' letter node. */
#define V(...) CH_DICT_NODE('v', __VA_ARGS__) /**< 'v' letter node. */
#define W(...) CH_DICT_NODE('w', __VA_ARGS__) /**< 'w' letter node. */
#define X(...) CH_DICT_NODE('x', __VA_ARGS__) /**< 'x' letter node. */
#define Y(...) CH_DICT_NODE('y', __VA_ARGS__) /**< 'y' letter node. */
#define Z(...) CH_DICT_NODE('z', __VA_ARGS__) /**< 'z' letter node. */

/**
 * @typedef ch_dict_cb
 * @brief Callback type for passing as an argument to ch_dict_foreach_word().
 * @param string Current string of the walkthrough.
 * @param data User data which was passed to ch_dict_foreach_word()
 */
typedef void (*ch_dict_cb)(const char *string, void *data);

/**
 * @brief Iterates through all the dictionary.
 * @param dict Dictionary to walk through.
 * @param cb Function called at each word of the dictionary.
 * @param data User data, passed back at each *cb* call.
 */
CUSHIONS_API void ch_dict_foreach_word(const struct ch_dict_node *dict,
		ch_dict_cb cb, void *data);

/**
 * @brief Checks if a dictionary contains a given string.
 * @param dict Dictionary in which the word will be searched.
 * @param str String to search in the dictionary.
 * @return true if the word is present in the dictionary, false otherwise.
 */
CUSHIONS_API bool dict_contains(const struct ch_dict_node *dict,
		const char *str);

#endif /* CUSHIONS_HANDLERS_DICT_H_ */

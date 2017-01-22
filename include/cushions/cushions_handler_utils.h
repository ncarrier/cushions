/*
 * one should not need to include this header directly, include
 * cushions_handler.h instead
 */
#ifndef CUSHIONS_HANDLER_UTILS_H
#define CUSHIONS_HANDLER_UTILS_H
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

/* only used for STRINGIFY implementation */
#define STRINGIFY_HELPER(s) #s
#define STRINGIFY(s) STRINGIFY_HELPER(s)

#ifndef container_of
#define container_of(ptr, type, member) ({ \
	const typeof(((type *)NULL)->member)*__mptr = (ptr); \
	(type *)((uintptr_t)__mptr - offsetof(type, member)); })
#endif /* container_of */

bool string_matches_prefix(const char *string, const char *prefix);
void string_cleanup(char **s);
void fd_cleanup(int *fd);

#endif /* CUSHIONS_HANDLER_UTILS_H */

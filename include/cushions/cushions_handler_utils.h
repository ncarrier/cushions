/*
 * one should not need to include this header directly, include
 * cushions_handler.h instead
 */
#ifndef CUSHIONS_HANDLER_UTILS_H
#define CUSHIONS_HANDLER_UTILS_H
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include <cushions_common.h>

/* only used for STRINGIFY implementation */
#define CH_STRINGIFY_HELPER(s) #s
#define CH_STRINGIFY(s) CH_STRINGIFY_HELPER(s)

#ifndef ch_container_of
#define ch_container_of(ptr, type, member) ({ \
	const typeof(((type *)NULL)->member)*__mptr = (ptr); \
	(type *)((uintptr_t)__mptr - offsetof(type, member)); })
#endif /* ch_container_of */

CUSHIONS_API bool ch_string_matches_prefix(const char *str, const char *prefix);
CUSHIONS_API void ch_string_cleanup(char **s);
CUSHIONS_API void ch_fd_cleanup(int *fd);

#endif /* CUSHIONS_HANDLER_UTILS_H */

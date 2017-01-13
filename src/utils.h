#ifndef _UTILS_H
#define _UTILS_H
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

#endif /* _UTILS_H */

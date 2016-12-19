#ifndef _UTILS_H
#define _UTILS_H
#include <stdbool.h>

/* only used for STRINGIFY implementation */
#define STRINGIFY_HELPER(s) #s
#define STRINGIFY(s) STRINGIFY_HELPER(s)

bool string_matches_prefix(const char *string, const char *prefix);
void string_cleanup(char **s);

#endif /* _UTILS_H */

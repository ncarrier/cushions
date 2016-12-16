#ifndef _UTILS_H
#define _UTILS_H
#include <stdbool.h>

bool string_matches_prefix(const char *string, const char *prefix);
void string_cleanup(char **s);

#endif /* _UTILS_H */

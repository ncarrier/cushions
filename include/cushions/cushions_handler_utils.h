/**
 * @file cushions_handler_utils.h
 * @brief Various utility definitions.
 *
 * One should not need to include this header directly, include
 * cushions_handler.h instead.
 */
#ifndef CUSHIONS_HANDLER_UTILS_H
#define CUSHIONS_HANDLER_UTILS_H
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include <cushions_common.h>

/** Only used for STRINGIFY implementation. */
#define CH_STRINGIFY_HELPER(s) #s

/**
 * @def CH_STRINGIFY
 * @brief Macro for converting it's argument into a string.
 */
#define CH_STRINGIFY(s) CH_STRINGIFY_HELPER(s)

/**
 * @def ch_container_of
 * @brief Implementation of the container_of macro, allows to retrieve the
 * object of type *type* for which we know the address *ptr* of the member
 * *member*.
 */
#ifndef ch_container_of
#define ch_container_of(ptr, type, member) ({ \
	const typeof(((type *)NULL)->member)*__mptr = (ptr); \
	(type *)((uintptr_t)__mptr - offsetof(type, member)); })
#endif /* ch_container_of */

/**
 * @brief Tells whether or not a string matches a given prefix.
 * @param str String to check.
 * @param prefix Prefix to test.
 * @return true if the string *string* starts with the prefix *prefix*.
 */
CUSHIONS_API bool ch_string_matches_prefix(const char *str, const char *prefix);

/**
 * @brief Destroys a string. Meant to be used as a cleanup variable attribute.
 * @param s In input, string to free, NULL in output. Can be NULL or point to
 * NULL.
 */
CUSHIONS_API void ch_string_cleanup(char **s);

/**
 * @brief Closes a file descriptor. Meant to be used as a cleanup variable
 * attribute.
 * @param fd In input, points to the file descriptor to close, points to -1 in
 * output. Can be NULL. If the pointed value is < 1, nothing will be done, to
 * avoid nasty bugs leading to closing stdin by mistake.
 */
CUSHIONS_API void ch_fd_cleanup(int *fd);

#endif /* CUSHIONS_HANDLER_UTILS_H */

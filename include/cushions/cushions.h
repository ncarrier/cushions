/**
 * @file cushions.h
 * @brief Main API, for clients willing to add URL-like support to their
 * program.
 *
 * Three solutions are available:
 *
 * * A client can call directly cushions_fopen().
 * * He/she can left his program untouched and link it with -Wl,--wrap=fopen.
 * * He can call the original fopen() function and launch the program with
 * the environment variable **LD_PRELOAD** set to **libcushions.so**.
 */
#ifndef _CUSHIONS_H
#define _CUSHIONS_H
#include <stdio.h>

#include <cushions_common.h>

/**
 * This function opens the file whose name is the string pointed to by *path*
 * and associates a stream with it.
 * @param path Name of the file to open, if it starts with SCHEME://, with
 * SCHEME being an URL scheme for which a scheme handler is registered, then
 * this handler will take care of the opening, otherwise, fopen will be used.
 * Multiple schemes can be chained provided the handler for the outermost
 * schemes do rely themselves on cushions_fopen.
 * @param mode Open mode string, such as "rbe", as described in fopen(3)
 * @return Returns a FILE pointer upon successful completion. Otherwise, NULL is
 * returned and errno is set to indicate the error.
 * @see fopen(3)
 */
CUSHIONS_API FILE *cushions_fopen(const char *path, const char *mode);

/**
 * Alias to cushions_fopen to use with linker option -Wl,--wrap=fopen.
 * @param path Path to open.
 * @param mode Open string mode.
 * @return Returns a FILE pointer upon successful completion. Otherwise, NULL is
 * returned and errno is set to indicate the error.
 * @see fopen(3)
 * @see cushions_fopen
 */
CUSHIONS_API FILE *__wrap_fopen(const char *path, const char *mode);

#endif /* _CUSHIONS_H */

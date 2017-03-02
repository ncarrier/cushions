/**
 * @file cushions_handler.h
 * @brief Main header for scheme handler implementors.
 *
 * People willing to implement a now scheme handler should include only this
 * header.
 *
 * To implement a new handler, one need mainly to register a ch_handler
 * structure, with its ch_handler.fopen operation implemented and register it with
 * ch_handler_register().
 * Other functions of this header and the other ones matching the
 * cushions_handler prefix, are mainly helper functions to ease the development
 * of new handlers.
 */
#ifndef _CUSHIONS_HANDLER_H
#define _CUSHIONS_HANDLER_H
#include <stdio.h>
#include <stdbool.h>

#include <cushions.h>
#include <cushions_handler_dict.h>
#include <cushions_handler_log.h>
#include <cushions_handler_utils.h>
#include <cushions_handler_mode.h>

/**
 * @struct ch_handler
 * @brief Main structure, represents a plugin, able to handle possibly multiple
 * URL schemes.
 */
struct ch_handler;

/**
 * @typedef ch_fopen
 * @brief Main function of a libcushions handler, responsible of opening a valid
 * libc stream, which can be used with, at least, fread, fwrite and fclose.
 * @param handler A pointer to the handler registered with
 * ch_handler_register().
 * @param path Path of the stream to open, with the SCHEME:// part stripped.
 * @param full_path Path including the SCHEME:// part.
 * @param scheme Scheme extracted from the full path.
 * @param mode Opening mode, broken out for easier manipulation.
 * @return FILE * stream pointer on success, NULL with errno set appropriately,
 * on errors.
 */
/* codecheck_ignore[NEW_TYPEDEFS,SPACING] */
typedef FILE *(*ch_fopen)(struct ch_handler *handler, const char *path,
		const char *full_path, const char *scheme,
		const struct ch_mode *mode);

/**
 * @typedef ch_handler_handles
 * @brief Function for deciding if a scheme handler accepts to handle a
 * particular scheme, or not.
 * @param handler A pointer to the handler registered with
 * ch_handler_register().
 * @param scheme Name of the scheme to test the support for.
 * @return true if the handler is able to handle this scheme, false otherwise.
 */
typedef bool (*ch_handler_handles)(struct ch_handler *handler,
		const char *scheme);

/**
 * @struct ch_handler
 * @brief Main structure, represents a plugin, able to handle possibly multiple
 * URL schemes.
 */
struct ch_handler {
	/**
	 * Name of the handler, doesn't necessarily correspond to the scheme it
	 * handles.
	 */
	const char *name;
	/**
	 * Main operation of the handler, mandatory.
	 *
	 * Must return a valid FILE * stream, suitable for fread / fwrite
	 * operations.
	 *
	 * Most of the time, it will be implementet upon fopencookie(3).
	 */
	ch_fopen fopen;
	/**
	 * Function used to determine whether or not, this handler handles a
	 * particular scheme.
	 *
	 * Optional, if NULL, the schemes will be compared with the name of the
	 * handler.
	 */
	ch_handler_handles handles;
};

/**
 * @brief Registers a new scheme handler in libcushions.
 * @param handler Handler to register, must stay "alive" during all the rest of
 * the program.
 * @return 0 on success, errno-compatible negative value on error.
 */
CUSHIONS_API int ch_handler_register(struct ch_handler *handler);

/**
 * @brief Calls the original libc's fopen implementation.
 * @param path Path of the file to open.
 * @param mode Open mode string.
 * @see fopen(3)
 * @return NULL on error, with errno set.
 */
CUSHIONS_API FILE *ch_handler_real_fopen(const char *path, const char *mode);

/**
 * @brief Extracts the rightmost part of a path, starting with a '?' char and
 * break it as a semi-colon separated of key=value pairs.
 * @param input URL to extract the parameter part from.
 * @param path In output, points to the URL without the parameters part. Must be
 * free()-ed after usage.
 * @param envz Envz array, which can be queried with envz_get(). Must be
 * free()-ed after usage.
 * @param envz_len Size of the envz.
 * @see man envz(3)
 * @return 0 on success, errno-compatible negative value on error.
 */
CUSHIONS_API int ch_break_params(const char *input, char **path, char **envz,
		size_t *envz_len);

#endif /* _CUSHIONS_HANDLER */

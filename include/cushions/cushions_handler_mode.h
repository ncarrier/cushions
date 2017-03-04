/**
 * @file cushions_handler_mode.h
 * @brief Utility functions for breaking up the mode fopen(3) argument into an
 * explicit set of boolean flags.
 *
 * One should not need to include this header directly, include
 * cushions_handler.h instead.
 */
#ifndef CUSHIONS_HANDLER_MODE_H_
#define CUSHIONS_HANDLER_MODE_H_
#include <stdbool.h>

#include <cushions_common.h>

/**
 * @struct ch_mode
 * @brief Represents a broken out mode string.
 */
struct ch_mode {
	/** true iif the file must be opened for reading */
	bool read:1;
	/**
	 * true iif the stream must be positionned at the beginning,
	 * writing may be done at the end, if append is true
	 */
	bool beginning:1;
	/** true iif the file must be opened for writing */
	bool write:1;
	/** true iif the file must be truncated */
	bool truncate:1;
	/** true iif the file must be created if it does not exist */
	bool create:1;
	/** true iif the file must be opened in binary mode */
	bool binary:1;
	/** true iif the file must be opened for appending (at the end) */
	bool append:1;
	/**
	 * from the manpage:
	 *    "Do not make the open operation, or subsequent read and write
	 *    operations, thread cancellation points."
	 */
	bool no_cancellation:1;
	/** the file must be closed after exec() call */
	bool cloexec:1;
	/** attempt to access the file with mmap */
	bool mmap:1;
	/** used with 'create', fail if the file already exist */
	bool excl:1;
	/** original mode string, if any */
	char *mode;
	/** coded character set to use, marks the stream as wide-oriented */
	char *ccs;
};

/**
 * @brief Breaks a mode string into a set of flags in a ch_mode structure.
 * @param mode In output, contains the broken flags of *str*. ch_mode_cleanup()
 * must be called when this structure is not to be used anymore.
 * @param str String to convert to a set of flags.
 * @return errno-compatible negative value or error, 0 on success.
 */
CUSHIONS_API int ch_mode_from_string(struct ch_mode *mode, const char *str);

/**
 * @brief Converts a ch_mode structure to a string suitable for passing as a
 * mode string to fopen et al..
 * @param mode Mode structure to convert.
 * @param str In output, points to the result of the conversion, which must be
 * free()-ed after usage.
 * @return errno-compatible negative value on error, 0 on success.
 */
CUSHIONS_API int ch_mode_to_string(const struct ch_mode *mode, char **str);

/**
 * @brief Frees the ressources allocated to a ch_mode structure after a call to
 * ch_mode_from_string().
 * @param mode Mod structure to perform the cleanup on.
 */
CUSHIONS_API void ch_mode_cleanup(struct ch_mode *mode);

/**
 * @brief Dumps a ch_mode structure's content in a human readable way in the
 * cushions log system, for debugging purpose.
 * @param mode Mode structure to dump.
 */
CUSHIONS_API void ch_mode_dump(const struct ch_mode *mode);

#endif /* CUSHIONS_HANDLER_MODE_H_ */

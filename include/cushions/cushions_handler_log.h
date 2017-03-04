/**
 * @file cushions_handler_log.h
 * @brief Utility functions for logging in the libcushions log system.
 *
 * Messages are logged to the standard error, provided their log level is above
 * the log level set in the CUSHIONS_LOG_LEVEL environment variable, at the
 * program's startup or in the last call to ch_log_set_level(), the latter
 * taking precedence over the former.
 *
 * If the CUSHIONS_LOG_NO_COLOR environment variable is set to anything
 * non-empty, then no ansi escape control sequence will be output for adding
 * colors to the log messages.
 *
 * One should not need to include this header directly, include
 * cushions_handler.h instead
 */
#ifndef CUSHIONS_HANDLER_LOG_H
#define CUSHIONS_HANDLER_LOG_H
#include <string.h>
#include <stdlib.h>

#include <cushions_handler_utils.h>
#include <cushions_common.h>

CUSHIONS_UNUSED
#ifndef CH_LOG_TAG
static const char *log_tag = "";
#else
/**
 * @var log_tag
 * @brief Tag prepended to all log lines logged by the module including this
 * header.
 */
static const char *log_tag = CH_STRINGIFY(CH_LOG_TAG);
#endif /* LOG_TAG */

/**
 * @def CH_ERROR
 * @brief Error log level for a message, it is the highest level.
 */
#define CH_ERROR 0

/**
 * @def CH_WARNING
 * @brief Error log level for a message.
 */
#define CH_WARNING 1

/**
 * @def CH_INFO
 * @brief Error log level for a message.
 */
#define CH_INFO 2

/**
 * @def CH_DEBUG
 * @brief Error log level for a message.
 */
#define CH_DEBUG 3

/**
 * @def LOG
 * @brief Logs a message at a given severity.
 * @param l Log level of the message, which is discarded if the level is under
 * the currently set log level, either by ch_log_set_level() or by the
 * CUSHIONS_LOG_NO_COLOR environment variable.
 */
#define LOG(l, ...) ch_log(log_tag, (l), __VA_ARGS__)

/**
 * @def LOGE
 * @brief Logs a message at the CH_ERROR level.
 */
#define LOGE(...) LOG(CH_ERROR, __VA_ARGS__)

/**
 * @def LOGW
 * @brief Logs a message at the CH_WARNING level.
 */
#define LOGW(...) LOG(CH_WARNING, __VA_ARGS__)

/**
 * @def LOGI
 * @brief Logs a message at the CH_INFO level.
 */
#define LOGI(...) LOG(CH_INFO, __VA_ARGS__)

/**
 * @def LOGD
 * @brief Logs a message at the CH_DEBUG level.
 */
#define LOGD(...) LOG(CH_DEBUG, __VA_ARGS__)

/**
 * @def LOGPE
 * @brief Logs a message at the CH_ERROR level, with the form "*s*: perror_msg".
 * @param s String of the message.
 * @param e Error which occurred, converted to a string message with strerror().
 */
#define LOGPE(s, e) LOGE("%s: %s", (s), strerror(abs((e))))

/**
 * @brief Logs a message. Avoid using it directly, use one of the LOGX macros
 * instead.
 * @param tag Log tag of the message.
 * @param level Level of the message to log.
 * @param fmt Format string of the message.
 */
CUSHIONS_API void ch_log(const char *tag,
		int level, const char *fmt, ...) CUHSIONS_PRINTF(3, 4);

/**
 * @brief Enables / disables the use of colors in the log messages. Default is
 * set to 'enable'.
 * @param enable True to enable color usage, false to disable it.
 */
CUSHIONS_API void ch_log_set_color(bool enable);

/**
 * @brief Sets the current log level of libcushions, messages with a log level
 * above this threshold will be discarded.
 * @param level New log level.
 */
CUSHIONS_API void ch_log_set_level(int level);

#endif /* CUSHIONS_HANDLER_LOG_H */

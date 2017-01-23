/*
 * one should not need to include this header directly, include
 * cushions_handler.h instead
 */
#ifndef CUSHIONS_HANDLER_LOG_H
#define CUSHIONS_HANDLER_LOG_H
#include <string.h>
#include <stdlib.h>

#include <cushions_handler_utils.h>
#include <cushions_common.h>

__attribute__((unused))
#ifndef LOG_TAG
static const char *log_tag = "";
#else
static const char *log_tag = STRINGIFY(LOG_TAG);
#endif /* LOG_TAG */

#define CUSHIONS_HANDLER_ERROR 0
#define CUSHIONS_HANDLER_WARNING 1
#define CUSHIONS_HANDLER_INFO 2
#define CUSHIONS_HANDLER_DEBUG 3

#define LOG(l, ...) cushions_handler_log(log_tag, (l), __VA_ARGS__)
#define LOGE(...) LOG(CUSHIONS_HANDLER_ERROR, __VA_ARGS__)
#define LOGW(...) LOG(CUSHIONS_HANDLER_WARNING, __VA_ARGS__)
#define LOGI(...) LOG(CUSHIONS_HANDLER_INFO, __VA_ARGS__)
#define LOGD(...) LOG(CUSHIONS_HANDLER_DEBUG, __VA_ARGS__)
#define LOGPE(s, e) LOGE("%s: %s", (s), strerror(abs((e))))

CUSHIONS_API __attribute__((format(printf, 3, 4)))
void cushions_handler_log(const char *tag, int level, const char *fmt, ...);
CUSHIONS_API void log_set_level(int level);

#endif /* CUSHIONS_HANDLER_LOG_H */

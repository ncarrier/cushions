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
#ifndef CH_LOG_TAG
static const char *log_tag = "";
#else
static const char *log_tag = CH_STRINGIFY(CH_LOG_TAG);
#endif /* LOG_TAG */

#define CH_ERROR 0
#define CH_WARNING 1
#define CH_INFO 2
#define CH_DEBUG 3

#define LOG(l, ...) ch_log(log_tag, (l), __VA_ARGS__)
#define LOGE(...) LOG(CH_ERROR, __VA_ARGS__)
#define LOGW(...) LOG(CH_WARNING, __VA_ARGS__)
#define LOGI(...) LOG(CH_INFO, __VA_ARGS__)
#define LOGD(...) LOG(CH_DEBUG, __VA_ARGS__)
#define LOGPE(s, e) LOGE("%s: %s", (s), strerror(abs((e))))

CUSHIONS_API __attribute__((format(printf, 3, 4))) void ch_log(const char *tag,
		int level, const char *fmt, ...);
CUSHIONS_API void ch_log_set_color(bool enable);
CUSHIONS_API void ch_log_set_level(int level);

#endif /* CUSHIONS_HANDLER_LOG_H */

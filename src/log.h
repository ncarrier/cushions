#ifndef _LOG_H
#define _LOG_H
#include <string.h>
#include <stdlib.h>

#include "utils.h"

#ifndef LOG_TAG
static const char *log_tag = "";
#else
static const char *log_tag = STRINGIFY(LOG_TAG);
#endif /* LOG_TAG */

#define CUSHION_HANDLER_ERROR 0
#define CUSHION_HANDLER_WARNING 1
#define CUSHION_HANDLER_INFO 2
#define CUSHION_HANDLER_DEBUG 3

#define LOG(l, ...) cushion_handler_log(log_tag, (l), __VA_ARGS__)
#define LOGE(...) LOG(CUSHION_HANDLER_ERROR, __VA_ARGS__)
#define LOGW(...) LOG(CUSHION_HANDLER_WARNING, __VA_ARGS__)
#define LOGI(...) LOG(CUSHION_HANDLER_INFO, __VA_ARGS__)
#define LOGD(...) LOG(CUSHION_HANDLER_DEBUG, __VA_ARGS__)
#define LOGPE(s, e) LOGE("%s: %s", (s), strerror(abs((e))))

__attribute__((format(printf, 3, 4)))
void cushion_handler_log(const char *tag, int level, const char *fmt, ...);
void log_set_level(int level);

#endif /* _LOG_H */

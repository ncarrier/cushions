#ifndef _LOG_H
#define _LOG_H
#include <string.h>
#include <stdlib.h>

#include "cushion_handler.h"

#define CUSHION_HANDLER_ERROR 0
#define CUSHION_HANDLER_WARNING 1
#define CUSHION_HANDLER_INFO 2
#define CUSHION_HANDLER_DEBUG 3

#define CH_LOG(l, ...) cushion_handler_log(NULL, (l), __VA_ARGS__)
#define CH_LOGE(...) CH_LOG(CUSHION_HANDLER_ERROR, __VA_ARGS__)
#define CH_LOGW(...) CH_LOG(CUSHION_HANDLER_WARNING, __VA_ARGS__)
#define CH_LOGI(...) CH_LOG(CUSHION_HANDLER_INFO, __VA_ARGS__)
#define CH_LOGD(...) CH_LOG(CUSHION_HANDLER_DEBUG, __VA_ARGS__)
#define CH_LOGPE(s, e) CH_LOGE("%s: %s", (s), strerror(abs((e))))

void cushion_handler_log(const struct cushion_handler *handler, int level,
		const char *fmt, ...);
void log_set_level(int level);

#endif /* _LOG_H */

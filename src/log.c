#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "log.h"

static int log_level = -1;

static void ch_vlog(const struct cushion_handler *h, int level,
		const char *fmt, va_list ap)
{
	static const char * const level_tags[] = {
		"[\e[1;31m %s E\e[0m] ",
		"[\e[1;33m %s W\e[0m] ",
		"[\e[1;35m %s I\e[0m] ",
		"[\e[1;36m %s D\e[0m] "
	};

	fprintf(stderr, level_tags[level], h != NULL ? h->scheme : "\b");
	vfprintf(stderr, fmt, ap);
	fputs("\n", stderr);
}

void cushion_handler_log(const struct cushion_handler *handler, int level,
		const char *fmt, ...)
{
	va_list ap;

	if (level > log_level)
		return;
	if (level > CUSHION_HANDLER_DEBUG)
		level = CUSHION_HANDLER_DEBUG;

	va_start(ap, fmt);
	ch_vlog(handler, level, fmt, ap);
	va_end(ap);
}

void log_set_level(int level)
{
	log_level = level;

	if (log_level > CUSHION_HANDLER_DEBUG)
		log_level = CUSHION_HANDLER_DEBUG;
}


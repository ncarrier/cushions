#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define LOG_TAG log
#include "log.h"

static int log_level = -1;

#define LOG_TAG_FORMAT "[\e[1;3%cm%c %10.10s\e[0m] "

static void ch_vlog(const char *tag, int level, const char *fmt, va_list ap)
{
	static const struct {
		char c;
		char l;
	} marks[] = {
		[CUSHIONS_HANDLER_ERROR] = {'1', 'E'},
		[CUSHIONS_HANDLER_WARNING] = {'3', 'W'},
		[CUSHIONS_HANDLER_INFO] = {'5', 'I'},
		[CUSHIONS_HANDLER_DEBUG] = {'6', 'D'},
	};

	if (level < 0)
		return;

	fprintf(stderr, LOG_TAG_FORMAT, marks[level].c, marks[level].l, tag);
	vfprintf(stderr, fmt, ap);
	fputs("\n", stderr);
}

void cushions_handler_log(const char *tag, int level, const char *fmt, ...)
{
	va_list ap;

	if (level > log_level)
		return;
	if (level > CUSHIONS_HANDLER_DEBUG)
		level = CUSHIONS_HANDLER_DEBUG;

	va_start(ap, fmt);
	ch_vlog(tag, level, fmt, ap);
	va_end(ap);
}

void log_set_level(int level)
{
	log_level = level;

	if (log_level > CUSHIONS_HANDLER_DEBUG)
		log_level = CUSHIONS_HANDLER_DEBUG;

	LOG(log_level, "%s(%d)", __func__, log_level);
}

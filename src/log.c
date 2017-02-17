#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define LOG_TAG log
#include "cushions_handler_log.h"

#define LOG_TAG_FORMAT_NO_COLOR "[%2$c %3$10.10s] \0%1$c"
#define LOG_TAG_FORMAT_COLOR "[\e[1;3%cm%c %10.10s\e[0m] "

static int log_level = -1;
static const char *tag_format = LOG_TAG_FORMAT_COLOR;

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	fprintf(stderr, tag_format, marks[level].c, marks[level].l, tag);
	vfprintf(stderr, fmt, ap);
#pragma GCC diagnostic pop
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

void log_set_color(bool enable)
{
	tag_format = enable ? LOG_TAG_FORMAT_COLOR : LOG_TAG_FORMAT_NO_COLOR;
}

void log_set_level(int level)
{
	log_level = level;

	if (log_level > CUSHIONS_HANDLER_DEBUG)
		log_level = CUSHIONS_HANDLER_DEBUG;

	LOG(log_level, "%s(%d)", __func__, log_level);
}

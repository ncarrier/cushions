#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define CH_LOG_TAG log
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
		[CH_ERROR] = {'1', 'E'},
		[CH_WARNING] = {'3', 'W'},
		[CH_INFO] = {'5', 'I'},
		[CH_DEBUG] = {'6', 'D'},
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

void ch_log(const char *tag, int level, const char *fmt, ...)
{
	va_list ap;

	if (level > log_level)
		return;
	if (level > CH_DEBUG)
		level = CH_DEBUG;

	va_start(ap, fmt);
	ch_vlog(tag, level, fmt, ap);
	va_end(ap);
}

void ch_log_set_color(bool enable)
{
	tag_format = enable ? LOG_TAG_FORMAT_COLOR : LOG_TAG_FORMAT_NO_COLOR;
}

void ch_log_set_level(int level)
{
	log_level = level;

	if (log_level > CH_DEBUG)
		log_level = CH_DEBUG;

	LOG(log_level, "%s(%d)", __func__, log_level);
}

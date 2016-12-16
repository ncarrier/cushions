#define _GNU_SOURCE
#include <argz.h>
#include <envz.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "cushion.h"
#include "cushion_handler.h"
#include "utils.h"

#define CH_LOG(l, ...) cushion_handler_log(NULL, (l), __VA_ARGS__)
#define CH_LOGE(...) CH_LOG(CUSHION_HANDLER_ERROR, __VA_ARGS__)
#define CH_LOGW(...) CH_LOG(CUSHION_HANDLER_WARNING, __VA_ARGS__)
#define CH_LOGI(...) CH_LOG(CUSHION_HANDLER_INFO, __VA_ARGS__)
#define CH_LOGD(...) CH_LOG(CUSHION_HANDLER_DEBUG, __VA_ARGS__)
#define CH_LOGPE(s, e) CH_LOGE("%s: %s", (s), strerror(abs((e))))

#define MAX_CUSHION_HANDLER 20
#define SCHEME_END_PATTERN "://"

static int log_level = -1;

static struct cushion_handler handlers[MAX_CUSHION_HANDLER];

/* Function pointers to hold the value of the glibc functions */
static FILE *(*real_fopen)(const char *path, const char *mode);

static int break_path(const char *path, char **scheme, char **envz,
		size_t *envz_len)
{
	int ret;
	const char *needle;
	const char *params;
	unsigned prefix_length;

	*envz = NULL;
	*envz_len = 0;
	*scheme = NULL;

	needle = strstr(path, "://");
	/*
	 * if no :// in path, or the part before is of length, there is no
	 * scheme in path
	 */
	if (needle == NULL || needle == path) {
		CH_LOGD("no scheme present in path");
		return 0;
	}
	prefix_length = needle - path + 3;

	*scheme = strdup(path);
	if (*scheme == NULL)
		return -errno;

	params = strrchr(*scheme, '?');
	/* cut the string at the end of the scheme */
	*(*scheme + (needle - path)) = '\0';

	CH_LOGD("there's only a scheme present, no parameters");
	if (params == NULL)
		return prefix_length;
	params++;

	ret = argz_create_sep(params, ';', envz, envz_len);
	if (ret < 0) {
		*envz = NULL;
		CH_LOGE("argz_create_sep failed");
		return -ENOMEM;
	}

	return prefix_length;
}

FILE *cushion_fopen(const char *path, const char *mode)
{
	int ret;
	int i;
	struct cushion_handler *h;
	char __attribute__((cleanup(string_cleanup)))*scheme = NULL;
	char __attribute__((cleanup(string_cleanup)))*envz = NULL;
	size_t envz_len;
	unsigned offset;

	offset = ret = break_path(path, &scheme, &envz, &envz_len);
	if (ret < 0) {
		CH_LOGPE("break_path", ret);
		errno = -ret;
		return NULL;
	}
	if (scheme != NULL) {
		for (i = 0; i < MAX_CUSHION_HANDLER; i++) {
			h = handlers + i;
			if (h->self == NULL)
				break;
			if (string_matches_prefix(path, h->scheme)) {
				CH_LOGI("%p handles scheme %s", h, h->scheme);
				return h->fopen(h, path + offset, mode, envz,
						envz_len);
			}
		}
	} else {
		CH_LOGD("no scheme detected, use real fopen");
		return real_fopen(path, mode);
	}

	CH_LOGI("no handler for scheme %s, fallback to real fopen", scheme);

	return real_fopen(path, mode);
}

static void ch_vlog(const struct cushion_handler *h, int level,
		const char *fmt, va_list ap)
{
	static const char * const level_tags[] = {
		"\e[1;31m %s E\e[0m ",
		"\e[1;33m %s W\e[0m ",
		"\e[1;35m %s I\e[0m ",
		"\e[1;36m %s D\e[0m "
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

int cushion_handler_register(const struct cushion_handler *handler)
{
	int i;

	if (handler == NULL || handler->fopen == NULL)
		return -EINVAL;

	for (i = 0; i < MAX_CUSHION_HANDLER; i++)
		if (handlers[i].self == NULL)
			break;

	if (i == MAX_CUSHION_HANDLER) {
		CH_LOGE("%s: too many handlers registered");
		return -ENOMEM;
	}

	handlers[i] = *handler;
	handlers[i].self = handler;

	return 0;
}

// TODO can these two be merged into one ?
FILE *__wrap_fopen(const char *path, const char *mode)
{
	return real_fopen(path, mode);
}

FILE *cushion_handler_real_fopen(const char *path, const char *mode)
{
	return real_fopen(path, mode);
}

__attribute__((constructor)) void cushion_constructor(void)
{
	const char *env_log_level;

	real_fopen = dlsym(RTLD_NEXT, "fopen");

	env_log_level = getenv("CUSHION_LOG_LEVEL");
	if (env_log_level != NULL)
		log_level = atoi(env_log_level);

	if (log_level > CUSHION_HANDLER_DEBUG)
		log_level = CUSHION_HANDLER_DEBUG;
}


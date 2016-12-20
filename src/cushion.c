#define _GNU_SOURCE
#include <argz.h>
#include <envz.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#define LOG_TAG cushion
#include "log.h"
#include "cushion_handler.h"
#include "utils.h"
#include "cushion_handlers.h"

#define MAX_CUSHION_HANDLER 20
#define SCHEME_END_PATTERN "://"

FILE *cushion_fopen(const char *path, const char *mode);

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
		LOGD("no scheme present in path");
		return 0;
	}
	prefix_length = needle - path + 3;

	*scheme = strdup(path);
	if (*scheme == NULL)
		return -errno;

	params = strrchr(*scheme, '?');
	/* cut the string at the end of the scheme */
	*(*scheme + (needle - path)) = '\0';

	LOGD("there's only a scheme present, no parameters");
	if (params == NULL)
		return prefix_length;
	params++;

	ret = argz_create_sep(params, ';', envz, envz_len);
	if (ret < 0) {
		*envz = NULL;
		LOGE("argz_create_sep failed");
		return -ENOMEM;
	}

	return prefix_length;
}

FILE *fopen(const char *path, const char *mode)
{
	return cushion_fopen(path, mode);
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
		LOGPE("break_path", ret);
		errno = -ret;
		return NULL;
	}
	if (scheme != NULL) {
		for (i = 0; i < MAX_CUSHION_HANDLER; i++) {
			h = handlers + i;
			if (h->self == NULL)
				break;
			if (string_matches_prefix(path, h->scheme)) {
				LOGI("%p handles scheme '%s'", h, h->scheme);
				return h->fopen(h, path + offset, mode, envz,
						envz_len);
			}
		}
	} else {
		LOGD("no scheme detected, use real fopen");
		return real_fopen(path, mode);
	}

	LOGI("no handler for scheme %s, fallback to real fopen", scheme);

	return real_fopen(path, mode);
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
		LOGE("%s: too many handlers registered", __func__);
		return -ENOMEM;
	}

	handlers[i] = *handler;
	handlers[i].self = handler;

	return 0;
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
	return cushion_fopen(path, mode);
}

FILE *cushion_handler_real_fopen(const char *path, const char *mode)
{
	return real_fopen(path, mode);
}

__attribute__((constructor)) void cushion_constructor(void)
{
	int ret;
	const char *env_log_level;

	real_fopen = dlsym(RTLD_NEXT, "fopen");

	env_log_level = getenv("CUSHION_LOG_LEVEL");
	if (env_log_level != NULL)
		log_set_level(atoi(env_log_level));

	ret = cushion_handlers_load();
	if (ret < 0)
		LOGW("cushion_handlers_load: %s", strerror(-ret));
}

__attribute__((destructor)) void cushion_destructor(void)
{
	cushion_handlers_unload();
}


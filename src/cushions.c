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
#include "cushions_handler.h"
#include "utils.h"
#include "cushions_handlers.h"

#define MAX_CUSHIONS_HANDLER 20
#define SCHEME_END_PATTERN "://"

FILE *cushions_fopen(const char *path, const char *mode);

static struct cushions_handler handlers[MAX_CUSHIONS_HANDLER];

/* Function pointers to hold the value of the glibc functions */
static FILE *(*real_fopen)(const char *path, const char *mode);

static int break_scheme(const char *path, char **scheme)
{
	const char *needle;
	unsigned prefix_length;

	*scheme = NULL;

	needle = strstr(path, "://");
	/*
	 * if no :// in path, or the part before is of length 0, there is no
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

	/* cut the string at the end of the scheme */
	*(*scheme + (needle - path)) = '\0';

	return prefix_length;
}

int cushions_handler_break_params(const char *input, char **path, char **envz,
		size_t *envz_len)
{
	int ret;
	char *params;

	if (input == NULL || path == NULL || envz == NULL || envz_len == NULL)
		return -EINVAL;

	*path = NULL;
	*envz = NULL;
	*envz_len = 0;

	params = strrchr(input, '?');
	if (params == NULL) {
		LOGD("no parameters are present");
		return 0;
	}
	*path = strdup(input);
	if (*path == NULL)
		return -errno;
	params = *path + (params - input);
	*params = '\0';
	params++;

	ret = argz_create_sep(params, ';', envz, envz_len);
	if (ret < 0) {
		*envz = NULL;
		LOGE("argz_create_sep failed");
		return -ENOMEM;
	}

	return 0;
}

FILE *fopen(const char *path, const char *mode)
{
	return cushions_fopen(path, mode);
}

FILE *cushions_fopen(const char *path, const char *mode)
{
	int ret;
	int i;
	struct cushions_handler *h;
	char __attribute__((cleanup(string_cleanup)))*scheme = NULL;
	char __attribute__((cleanup(string_cleanup)))*envz = NULL;
	unsigned offset;

	offset = ret = break_scheme(path, &scheme);
	if (ret < 0) {
		LOGPE("break_path", ret);
		errno = -ret;
		return NULL;
	}
	if (scheme != NULL) {
		for (i = 0; i < MAX_CUSHIONS_HANDLER; i++) {
			h = handlers + i;
			if (h->self == NULL)
				break;
			if (string_matches_prefix(path, h->scheme)) {
				LOGI("%p handles scheme '%s'", h, h->scheme);
				return h->fopen(h, path + offset, mode);
			}
		}
	} else {
		LOGD("no scheme detected, use real fopen");
		return real_fopen(path, mode);
	}

	LOGI("no handler for scheme %s, fallback to real fopen", scheme);

	return real_fopen(path, mode);
}

int cushions_handler_register(const struct cushions_handler *handler)
{
	int i;

	if (handler == NULL || handler->fopen == NULL)
		return -EINVAL;

	for (i = 0; i < MAX_CUSHIONS_HANDLER; i++)
		if (handlers[i].self == NULL)
			break;

	if (i == MAX_CUSHIONS_HANDLER) {
		LOGE("%s: too many handlers registered", __func__);
		return -ENOMEM;
	}

	handlers[i] = *handler;
	handlers[i].self = handler;

	return 0;
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
	return cushions_fopen(path, mode);
}

FILE *cushions_handler_real_fopen(const char *path, const char *mode)
{
	return real_fopen(path, mode);
}

__attribute__((constructor)) void cushions_constructor(void)
{
	int ret;
	const char *env_log_level;

	real_fopen = dlsym(RTLD_NEXT, "fopen");

	env_log_level = getenv("CUSHIONS_LOG_LEVEL");
	if (env_log_level != NULL)
		log_set_level(atoi(env_log_level));

	ret = cushions_handlers_load();
	if (ret < 0)
		LOGW("cushions_handlers_load: %s", strerror(-ret));
}

__attribute__((destructor)) void cushions_destructor(void)
{
	cushions_handlers_unload();
}

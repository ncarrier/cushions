#define _GNU_SOURCE
#include <argz.h>
#include <envz.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <cushions.h>

#define LOG_TAG cushion
#include "cushions_handler.h"

#define MAX_CUSHIONS_HANDLER 20
#define SCHEME_END_PATTERN "://"

static struct cushions_handler *handlers[MAX_CUSHIONS_HANDLER];

/* Function pointers to hold the value of the glibc functions */
static FILE *(*real_fopen)(const char *path, const char *mode);

static int break_scheme(const char *path, char **scheme)
{
	const char *needle;
	unsigned prefix_length;

	*scheme = NULL;

	needle = strstr(path, SCHEME_END_PATTERN);
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

static bool handle_handles(struct cushions_handler *handler, const char *scheme)
{
	if (handler->handles == NULL)
		return string_matches_prefix(scheme, handler->name);
	else
		return handler->handles(handler, scheme);
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

FILE *cushions_fopen(const char *path, const char *m)
{
	int ret;
	int i;
	struct cushions_handler *h;
	char __attribute__((cleanup(string_cleanup)))*scheme = NULL;
	char __attribute__((cleanup(string_cleanup)))*envz = NULL;
	unsigned offset;
	struct cushions_handler_mode
	__attribute__((cleanup(cushions_handler_mode_cleanup)))mode = {
			.ccs = NULL,
	};

	LOGD("%s(%s, %s)", __func__, path, m);

	offset = ret = break_scheme(path, &scheme);
	if (ret < 0) {
		LOGPE("break_path", ret);
		errno = -ret;
		return NULL;
	}
	if (scheme == NULL) {
		LOGD("no scheme detected, use real fopen");
		return real_fopen(path, m);
	}

	for (i = 0; i < MAX_CUSHIONS_HANDLER; i++) {
		h = handlers[i];
		if (h == NULL)
			break;
		if (handle_handles(h, scheme)) {
			LOGI("%s handles scheme '%s'", h->name, scheme);
			ret = cushions_handler_mode_from_string(&mode, m);
			if (ret < 0) {
				LOGPE("cushions_handler_mode_from_string", ret);
				errno = -ret;
				return NULL;
			}
			return h->fopen(h, path + offset, path, scheme, &mode);
		}
	}

	LOGI("no handler for scheme %s, fallback to real fopen", scheme);

	return real_fopen(path, m);
}

int cushions_handler_register(struct cushions_handler *handler)
{
	int i;

	if (handler == NULL || handler->fopen == NULL)
		return -EINVAL;

	for (i = 0; i < MAX_CUSHIONS_HANDLER; i++)
		if (handlers[i] == NULL)
			break;

	if (i == MAX_CUSHIONS_HANDLER) {
		LOGE("%s: too many handlers registered", __func__);
		return -ENOMEM;
	}

	handlers[i] = handler;

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

static __attribute__((constructor)) void cushions_constructor(void)
{
	const char *env;

	real_fopen = dlsym(RTLD_NEXT, "fopen");

	env = getenv("CUSHIONS_LOG_NO_COLOR");
	if (env != NULL)
		log_set_color(false);
	env = getenv("CUSHIONS_LOG_LEVEL");
	if (env != NULL)
		log_set_level(atoi(env));
}

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <curl/curl.h>

#include <cushions.h>
#include <cushions_handler.h>

#define LOG_TAG curl_handler
#include "log.h"

#include "dict.h"

#define BUFFER_SIZE 0x400

enum fcurl_type_e {
	CFTYPE_NONE = 0,
	CFTYPE_FILE = 1,
	CFTYPE_CURL = 2
};

struct curl_cushions_handler {
	struct cushions_handler handler;
	cookie_io_functions_t curl_func;
};

static const struct dict_node dict = DICT(F(T(P(_))),
                                          S(C(P(_)),
                                            M(B(S(_),
                                                  _)),
                                          F(T(P(_)))),
                                          H(T(T(P(S(_),
                                                  _)))),
                                          T(F(T(P(_)))));

struct curl_cushions_file {
	enum fcurl_type_e type; /* type of handle */
	union {
		CURL *curl;
		FILE *file;
	} handle; /* handle */

	char *buffer; /* buffer to store cached data*/
	size_t buffer_len; /* currently allocated buffers length */
	size_t buffer_pos; /* end of data in buffer*/
	/*
	 * still_running must stay an int since that's what curl_multi_perfom
	 * wants
	 */
	int still_running; /* Is background url fetch still in progress */
};

static const struct curl_cushions_handler curl_cushions_handler;

static int curl_close(void *c)
{

	return -1;
}

static FILE *curl_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct cushions_handler_mode *mode)
{
	/* TODO less restrictive condition */
	if (strcmp(mode->mode, "r") != 0 && strcmp(mode->mode, "rb") != 0) {
		LOGE("curl scheme only supports \"r\" open mode");
		errno = EINVAL;
		return NULL;
	}
	errno = ENOSYS;

	return NULL;
}

static ssize_t curl_read(void *c, char *buf, size_t size)
{

	return -1;
}

static bool curl_handler_handles(struct cushions_handler *handler,
		const char *scheme)
{
	return dict_contains(&dict, scheme);
}

static const struct curl_cushions_handler curl_cushions_handler = {
		.handler = {
				/* not used for matching scheme */
				.scheme = "curl",
				.fopen = curl_cushions_fopen,
				.handles = curl_handler_handles,
		},
		.curl_func = {
				.read  = curl_read,
				.close = curl_close
		},
};

static __attribute__((constructor)) void curl_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = cushions_handler_register(&curl_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(curl_cushions_handler): %s",
				strerror(-ret));
}

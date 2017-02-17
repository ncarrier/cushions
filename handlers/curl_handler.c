/*
 * Adapted from the fopen.c example provided in the curl website:
 *     https://curl.haxx.se/libcurl/c/fopen.html
 *
 * The original copyrigth notice is reproduced here:
 *
 * Copyright (c) 2003 Simtec Electronics
 *
 * Re-implemented by Vincent Sanders <vince@kyllikki.org> with extensive
 * reference to original curl example code
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _GNU_SOURCE
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <curl/curl.h>

#define CH_LOG_TAG curl_handler
#include <cushions_handler.h>

#define BUFFER_SIZE 0x400

struct curl_cushions_handler {
	struct ch_handler handler;
	cookie_io_functions_t curl_func;
	/* global, should be mutexed for multithread context */
	CURLM *multi_handle;
};

#define to_curl_handler(h) ch_container_of(h, struct curl_cushions_handler, \
		handler)

static const struct ch_dict_node dict = CH_DICT(F(T(P(_))),
                                                S(C(P(_)),
                                                  M(B(S(_),
                                                      _)),
                                                F(T(P(_)))),
                                                H(T(T(P(S(_),
                                                        _)))),
                                                T(F(T(P(_)))));

struct curl_cushions_file {
	CURL *curl;

	char *buffer; /* buffer to store cached data*/
	size_t buffer_len; /* currently allocated buffers length */
	size_t buffer_pos; /* end of data in buffer*/
	/*
	 * still_running must stay an int since that's what curl_multi_perfom
	 * wants
	 */
	int still_running; /* Is background url fetch still in progress */
	struct curl_cushions_handler *handler;
	char err_buf[CURL_ERROR_SIZE];
	bool handler_added;
};

static struct curl_cushions_handler curl_cushions_handler;

static int curl_close(void *c)
{
	struct curl_cushions_file *file;

	file = c;

	if (file->handler_added)
		curl_multi_remove_handle(file->handler->multi_handle,
				file->curl);
	if (file->curl != NULL)
		curl_easy_cleanup(file->curl);
	if (file->buffer != NULL)
		free(file->buffer);
	memset(file, 0, sizeof(*file));
	free(file);

	return 0;
}

/* curl calls this routine to get more data */
static size_t write_callback(char *buffer, size_t size, size_t nitems,
		void *userp)
{
	char *newbuff;
	size_t rembuff;
	struct curl_cushions_file *url;

	url = userp;
	size *= nitems;

	rembuff = url->buffer_len - url->buffer_pos;

	if (size > rembuff) {
		/* not enough space in buffer */
		newbuff = realloc(url->buffer,
				url->buffer_len + (size - rembuff));
		if (newbuff == NULL) {
			LOGPE("callback realloc", errno);
			size = rembuff;
		} else {
			/* realloc succeeded increase buffer size*/
			url->buffer_len += size - rembuff;
			url->buffer = newbuff;
		}
	}

	memcpy(&url->buffer[url->buffer_pos], buffer, size);
	url->buffer_pos += size;

	return size;
}

static FILE *curl_cushions_fopen(struct ch_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct ch_mode *mode)
{
	struct curl_cushions_file *file;
	struct curl_cushions_handler *ch;
	CURLMcode mcode;
	CURLcode ecode;

	ch = to_curl_handler(handler);

	/* TODO less restrictive condition */
	if (strcmp(mode->mode, "r") != 0 && strcmp(mode->mode, "rb") != 0) {
		LOGE("curl scheme only supports \"r\" open mode");
		errno = EINVAL;
		return NULL;
	}

	file = calloc(1, sizeof(*file));
	if (file == NULL)
		return NULL;

	file->curl = curl_easy_init();
	if (file->curl == NULL) {
		LOGE("curl_easy_init failed");
		goto err;
	}
	file->handler = ch;

	ecode = curl_easy_setopt(file->curl, CURLOPT_URL, full_path);
	if (ecode != CURLE_OK) {
		LOGE("curl_easy_setopt url: %s, %s", curl_easy_strerror(ecode),
				file->err_buf);
		goto err;
	}
	ecode = curl_easy_setopt(file->curl, CURLOPT_ERRORBUFFER,
			file->err_buf);
	if (ecode != CURLE_OK) {
		LOGE("curl_easy_setopt write_data: %s, %s",
				curl_easy_strerror(ecode), file->err_buf);
		goto err;
	}
	ecode = curl_easy_setopt(file->curl, CURLOPT_WRITEDATA, file);
	if (ecode != CURLE_OK) {
		LOGE("curl_easy_setopt url: %s, %s", curl_easy_strerror(ecode),
				file->err_buf);
		goto err;
	}
	ecode = curl_easy_setopt(file->curl, CURLOPT_VERBOSE, 0L);
	if (ecode != CURLE_OK) {
		LOGE("curl_easy_setopt verbose: %s, %s",
				curl_easy_strerror(ecode), file->err_buf);
		goto err;
	}
	ecode = curl_easy_setopt(file->curl, CURLOPT_WRITEFUNCTION,
			write_callback);
	if (ecode != CURLE_OK) {
		LOGE("curl_easy_setopt writefunction: %s, %s",
				curl_easy_strerror(ecode), file->err_buf);
		goto err;
	}

	if (ch->multi_handle == NULL) {
		ch->multi_handle = curl_multi_init();
		if (ch->multi_handle == NULL) {
			LOGE("%d: curl_multi_init failed", __LINE__);
			goto err;
		}
	}

	mcode = curl_multi_add_handle(ch->multi_handle, file->curl);
	if (mcode != CURLM_OK) {
		LOGE("curl_multi_add_handle: %s", curl_multi_strerror(mcode));
		goto err;
	}
	file->handler_added = true;

	/* lets start the fetch */
	mcode = curl_multi_perform(ch->multi_handle, &file->still_running);
	if (mcode != CURLM_OK) {
		LOGE("curl_multi_perform: %s", curl_multi_strerror(mcode));
		goto err;
	}

	if (file->buffer_pos != 0 || file->still_running)
		return fopencookie(file, mode->mode,
				curl_cushions_handler.curl_func);

err:
	curl_close(file);

	errno = EIO;

	return NULL;
}

/* use to attempt to fill the read buffer up to requested number of bytes */
static int fill_buffer(struct curl_cushions_file *file, size_t want)
{
	fd_set fdread;
	fd_set fdwrite;
	fd_set fdexcep;
	struct timeval timeout;
	int rc;
	CURLMcode mc;
	int maxfd;
	long curl_timeo;

	/*
	 * only attempt to fill buffer if transactions still running and buffer
	 * doesn't exceed required size already
	 */
	if ((!file->still_running) || (file->buffer_pos > want))
		return 0;

	do {

		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdexcep);

		timeout.tv_sec = 60;
		timeout.tv_usec = 0;

		mc = curl_multi_timeout(file->handler->multi_handle,
				&curl_timeo);
		if (mc != CURLM_OK) {
			LOGE("curl_multi_timeout: %s", curl_multi_strerror(mc));
			break;
		}
		if (curl_timeo >= 0) {
			timeout.tv_sec = curl_timeo / 1000;
			if (timeout.tv_sec > 1)
				timeout.tv_sec = 1;
			else
				timeout.tv_usec = (curl_timeo % 1000) * 1000;
		}

		/* get file descriptors from the transfers */
		mc = curl_multi_fdset(file->handler->multi_handle, &fdread,
				&fdwrite, &fdexcep, &maxfd);
		if (mc != CURLM_OK) {
			LOGE("curl_multi_fdset: %s", curl_multi_strerror(mc));
			break;
		}

		/*
		 * On success the value of maxfd is guaranteed to be >= -1. We
		 * call select(maxfd + 1, ...); specially in case of
		 * (maxfd == -1) there are no fds ready yet so sleep 100ms,
		 * which is the minimum suggested value in the
		 * curl_multi_fdset() doc.
		 */
		if (maxfd == -1) {
			usleep(100 * 1000);
		} else {
			rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep,
					&timeout);
			if (rc == -1)
				return -errno;
		}
		/* timeout or readable/writable sockets */
		mc = curl_multi_perform(file->handler->multi_handle,
				&file->still_running);
		if (mc != CURLM_OK) {
			LOGE("curl_multi_perform: %s", curl_multi_strerror(mc));
			break;
		}
	} while (file->still_running && (file->buffer_pos < want));

	return 0;
}

/* use to remove want bytes from the front of a files buffer */
static void use_buffer(struct curl_cushions_file *file, size_t want)
{
	if (file->buffer_pos - want <= 0) {
		file->buffer_pos = 0;
	} else {
		/* move rest down make it available for later */
		memmove(file->buffer, &file->buffer[want],
				(file->buffer_pos - want));

		file->buffer_pos -= want;
	}
}

static ssize_t curl_read(void *c, char *buf, size_t size)
{
	int ret;
	struct curl_cushions_file *file;

	file = c;
	ret = fill_buffer(file, size);
	if (ret < 0) {
		LOGPE("fill_buffer", ret);
		errno = -ret;
		return -1;
	}

	/* check if there's data in the buffer, if not -> EOF */
	if (file->buffer_pos == 0)
		return 0;

	/* ensure only available data is considered */
	if (file->buffer_pos < size)
		size = file->buffer_pos;

	/* transfer data to caller */
	memcpy(buf, file->buffer, size);

	use_buffer(file, size);

	return size;
}

static bool curl_handler_handles(struct ch_handler *handler,
		const char *scheme)
{
	return dict_contains(&dict, scheme);
}

static struct curl_cushions_handler curl_cushions_handler = {
		.handler = {
				/* not used for matching scheme */
				.name = "curl",
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
	CURLMcode mc;

	LOGI(__func__);

	mc = curl_global_init(CURL_GLOBAL_ALL);
	if (mc != CURLM_OK) {
		LOGE("curl_global_init: %s", curl_multi_strerror(mc));
		return;
	}

	ret = ch_handler_register(&curl_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(curl_cushions_handler): %s",
				strerror(-ret));
}

static __attribute__((destructor)) void curl_cushions_handler_destructor(
		void)
{
	LOGI(__func__);

	if (curl_cushions_handler.multi_handle != NULL)
		curl_multi_cleanup(curl_cushions_handler.multi_handle);

	curl_global_cleanup();
}

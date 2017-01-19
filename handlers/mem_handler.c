#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>

#include <cushions.h>
#include <cushions_handler.h>

#define LOG_TAG mem_handler
#include "log.h"

struct mem_cushions_handler {
	struct cushions_handler handler;
	cookie_io_functions_t mem_func;
};

#define to_mem_handler(h) container_of((h), struct mem_cushions_handler, handler)

struct mem_cushions_file {
	FILE *dest_file;
	FILE *mem_file;
	char *buffer;
	size_t size;
};

static int mem_close(void *cookie)
{
	int old_errno = 0;
	int ret;
	ssize_t sret;
	struct mem_cushions_file *mf = cookie;

	if (mf == NULL) {
		errno = EINVAL;
		return EOF;
	}

	if (mf->mem_file != NULL) {
		ret = fclose(mf->mem_file);
		if (ret == EOF) {
			old_errno = errno;
			LOGPE("fclose", old_errno);
			mf->mem_file = NULL;
			mem_close(cookie);
			errno = old_errno;
			return EOF;
		}
	}
	if (mf->dest_file == NULL) {
		errno = EINVAL;
		return EOF;
	} else {
		if (mf->buffer != NULL) {
			LOGI("write all data at once");
			sret = fwrite(mf->buffer, mf->size, 1, mf->dest_file);
			if (sret < 1) {
				old_errno = errno;
				LOGPE("fwrite", ret);
			}
		}
		ret = fclose(mf->dest_file);
		if (ret == EOF) {
			if (old_errno == 0)
				old_errno = errno;
			LOGPE("fclose", old_errno);
		}
	}
	if (mf->buffer != NULL)
		free(mf->buffer);

	free(cookie);

	if (old_errno != 0) {
		errno = old_errno;
		return EOF;
	}

	return 0;
}

static FILE *mem_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct cushions_handler_mode *mode)
{
	int old_errno;
	struct mem_cushions_file *mf;
	struct mem_cushions_handler *h = to_mem_handler(handler);

	/* TODO less restrictive condition */
	if (strcmp(mode->mode, "w") != 0 && strcmp(mode->mode, "wb") != 0) {
		LOGE("lzo scheme only supports \"r\" open mode");
		errno = EINVAL;
		return NULL;
	}

	mf = calloc(1, sizeof(*mf));
	if (mf == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	mf->dest_file = cushions_fopen(path, mode->mode);
	if (mf->dest_file == NULL) {
		old_errno = errno;
		LOGPE("cushions_fopen", errno);
		goto err;
	}

	mf->mem_file = open_memstream(&mf->buffer, &mf->size);
	if (mf->dest_file == NULL) {
		old_errno = errno;
		LOGPE("open_memstream", errno);
		goto err;
	}

	/* TODO adapt mode according to the mode argument */
	return fopencookie(mf, mode->mode, h->mem_func);
err:

	mem_close(mf);

	errno = old_errno;

	return NULL;
}

static ssize_t mem_write(void *cookie, const char *buf, size_t size)
{
	ssize_t sret;
	struct mem_cushions_file *mf = cookie;

	sret = fwrite(buf, size, 1, mf->mem_file);
	if (sret < 0) {
		LOGPE("fwrite", errno);
		return 0;
	}

	return size;
}

static struct mem_cushions_handler mem_cushions_handler = {
	.handler = {
		.name = "mem",
		.fopen = mem_cushions_fopen,
	},
	.mem_func = {
		.write = mem_write,
		.close = mem_close
	},
};

static __attribute__((constructor)) void lzo_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = cushions_handler_register(&mem_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(mem_cushions_handler): %s",
				strerror(-ret));
}

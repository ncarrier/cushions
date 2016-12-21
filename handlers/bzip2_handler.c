#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <bzlib.h>

#include <cushions.h>
#include <cushions_handler.h>

#include <bzlib.h>

#define LOG_TAG bzip2_handler
#include "log.h"

#define BUFFER_SIZE 0x400

struct bzip2_cushions_handler {
	struct cushions_handler handler;
	cookie_io_functions_t bzip2_func;
};

struct bzip2_cushions_file {
	int error;
	FILE *file;
	BZFILE *bz;
	char buffer[BUFFER_SIZE];
	bool eof;
};

static const struct bzip2_cushions_handler bzip2_cushions_handler;

static int bzip2_close(void *c)
{
	struct bzip2_cushions_file *bz2_c_file = c;

	if (bz2_c_file->bz != NULL)
		BZ2_bzReadClose(&bz2_c_file->error, bz2_c_file->bz);
	if (bz2_c_file->file != NULL)
		fclose(bz2_c_file->file);
	memset(bz2_c_file, 0, sizeof(*bz2_c_file));
	free(bz2_c_file);

	return 0;
}

static FILE *bzip2_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *mode)
{
	int old_errno;
	struct bzip2_cushions_file *bz2_c_file;

	LOGD(__func__);

	/* TODO less restrictive condition */
	if (strcmp(mode, "r") != 0 && strcmp(mode, "rb") != 0) {
		LOGE("bzip2 scheme only supports \"r\" open mode");
		errno = EINVAL;
		return NULL;
	}

	bz2_c_file = calloc(1, sizeof(*bz2_c_file));
	if (bz2_c_file == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	bz2_c_file->file = cushions_fopen(path, mode);
	if (bz2_c_file->file == NULL) {
		old_errno = errno;
		LOGPE("cushions_fopen", errno);
		goto err;
	}
	bz2_c_file->bz = BZ2_bzReadOpen(&bz2_c_file->error, bz2_c_file->file,
			0, 0, NULL, 0);
	if (bz2_c_file->bz == NULL) {
		old_errno = EIO;
		LOGE("BZ2_bzReadOpen error %s(%d)",
				BZ2_bzerror(bz2_c_file->bz, &bz2_c_file->error),
				bz2_c_file->error);
		goto err;
	}

	/* TODO adapt mode according to the mode argument */
	return fopencookie(bz2_c_file, mode, bzip2_cushions_handler.bzip2_func);
err:

	bzip2_close(bz2_c_file);

	errno = old_errno;
	return NULL;
}

static ssize_t bzip2_write(void *c, const char *buf, size_t size)
{

	errno = ENOSYS;
	return -1;
}

static ssize_t bzip2_read(void *c, char *buf, size_t size)
{
	int ret;
	struct bzip2_cushions_file *bz2_c_file = c;

	if (bz2_c_file->eof)
		return 0;

	ret = BZ2_bzRead(&bz2_c_file->error, bz2_c_file->bz, buf, size);
	if (bz2_c_file->error < BZ_OK) {
		LOGE("BZ2_bzRead error %s(%d)",
				BZ2_bzerror(bz2_c_file->bz, &bz2_c_file->error),
				bz2_c_file->error);
		errno = EIO;
		return -1;
	}
	if (bz2_c_file->error == BZ_STREAM_END)
		bz2_c_file->eof = true;

	return ret;
}

static int bzip2_seek(void *c, off64_t *offset, int whence)
{

	errno = ENOSYS;
	return -1;
}

static const struct bzip2_cushions_handler bzip2_cushions_handler = {
	.handler = {
		.scheme = "bzip2",
		.fopen = bzip2_cushions_fopen,
	},
	.bzip2_func = {
		.read  = bzip2_read,
		.write = bzip2_write,
		.seek  = bzip2_seek,
		.close = bzip2_close
	},
};

__attribute__((constructor)) void bzip2_cushions_handler_constructor(void)
{
	int ret;

	LOGI(__func__);

	ret = cushions_handler_register(&bzip2_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(bzip2_cushions_handler): %s",
				strerror(-ret));
}

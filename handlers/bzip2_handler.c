#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <bzlib.h>

#define CH_LOG_TAG bzip2_handler
#include <cushions_handler.h>

#define BUFFER_SIZE 0x400

struct bzip2_cushions_handler {
	struct ch_handler handler;
	cookie_io_functions_t bzip2_func;
};

enum direction {
	READ,
	WRITE,
};

struct bzip2_cushions_file {
	int error;
	FILE *file;
	BZFILE *bz;
	char buffer[BUFFER_SIZE];
	bool eof;
	enum direction direction;
};

static struct bzip2_cushions_handler bzip2_cushions_handler;

static int bzip2_close(void *c)
{
	struct bzip2_cushions_file *bz2_c_file = c;

	if (bz2_c_file->bz != NULL) {
		if (bz2_c_file->direction == READ)
			BZ2_bzReadClose(&bz2_c_file->error, bz2_c_file->bz);
		else
			BZ2_bzWriteClose(&bz2_c_file->error, bz2_c_file->bz, 0,
					NULL, NULL);
	}
	if (bz2_c_file->file != NULL)
		fclose(bz2_c_file->file);
	memset(bz2_c_file, 0, sizeof(*bz2_c_file));
	free(bz2_c_file);

	return 0;
}

static bool mode_is_valid(const struct ch_mode *mode)
{
	if (mode->append)
		return false;

	if (mode->read && mode->write)
		return false;

	return mode->read || mode->write;
}

static FILE *bzip2_cushions_fopen(struct ch_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct ch_mode *mode)
{
	int old_errno;
	struct bzip2_cushions_file *bz2_c_file;

	LOGD(__func__);

	if (!mode_is_valid(mode)) {
		errno = EINVAL;
		return NULL;
	}
	if (!mode->binary)
		LOGW("binary mode not set, may cause problems on some OSes");

	bz2_c_file = calloc(1, sizeof(*bz2_c_file));
	if (bz2_c_file == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	bz2_c_file->file = cushions_fopen(path, mode->mode);
	if (bz2_c_file->file == NULL) {
		old_errno = errno;
		LOGPE("cushions_fopen", errno);
		goto err;
	}
	if (mode->read) {
		bz2_c_file->direction = READ;
		bz2_c_file->bz = BZ2_bzReadOpen(&bz2_c_file->error,
				bz2_c_file->file, 0, 0, NULL, 0);
		if (bz2_c_file->bz == NULL) {
			old_errno = EIO;
			LOGE("BZ2_bzReadOpen error %s(%d)",
					BZ2_bzerror(bz2_c_file->bz,
							&bz2_c_file->error),
					bz2_c_file->error);
			goto err;
		}
	} else {
		bz2_c_file->direction = WRITE;
		bz2_c_file->bz = BZ2_bzWriteOpen(&bz2_c_file->error,
				bz2_c_file->file, 9, 0, 0);
		if (bz2_c_file->bz == NULL) {
			old_errno = EIO;
			LOGE("BZ2_bzWriteOpen error %s(%d)",
					BZ2_bzerror(bz2_c_file->bz,
							&bz2_c_file->error),
					bz2_c_file->error);
			goto err;
		}
	}

	return fopencookie(bz2_c_file, mode->mode,
			bzip2_cushions_handler.bzip2_func);
err:

	bzip2_close(bz2_c_file);

	errno = old_errno;
	return NULL;
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

static ssize_t bzip2_write(void *cookie, const char *buf, size_t size)
{
	struct bzip2_cushions_file *bz2_c_file = cookie;

	if (bz2_c_file->eof)
		return 0;

	/* buf parameter of BZ2_bzWrite isn't const, don't know why... */
	BZ2_bzWrite(&bz2_c_file->error, bz2_c_file->bz, (void *)buf, size);
	if (bz2_c_file->error < BZ_OK) {
		LOGE("BZ2_bzWrite error %s(%d)",
				BZ2_bzerror(bz2_c_file->bz, &bz2_c_file->error),
				bz2_c_file->error);
		errno = EIO;
		return 0;
	}
	if (bz2_c_file->error == BZ_STREAM_END) {
		bz2_c_file->eof = true;
		return 0;
	}

	return size;
}

static struct bzip2_cushions_handler bzip2_cushions_handler = {
	.handler = {
		.name = "bzip2",
		.fopen = bzip2_cushions_fopen,
	},
	.bzip2_func = {
		.read  = bzip2_read,
		.write = bzip2_write,
		.close = bzip2_close
	},
};

static __attribute__((constructor)) void bzip2_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = ch_handler_register(&bzip2_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(bzip2_cushions_handler): %s",
				strerror(-ret));
}

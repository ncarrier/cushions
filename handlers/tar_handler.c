#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <tar.h>
//#include <bzlib.h>

#define LOG_TAG tar_handler
#include <cushions_handler.h>

//#define BUFFER_SIZE 0x400
//
struct tar_cushions_handler {
	struct cushions_handler handler;
	cookie_io_functions_t tar_func;
};

//enum direction {
//	READ,
//	WRITE,
//};
//
struct tar_cushions_file {
//	int error;
//	FILE *file;
//	BZFILE *bz;
//	char buffer[BUFFER_SIZE];
//	bool eof;
//	enum direction direction;
};

static struct tar_cushions_handler tar_cushions_handler;

static int tar_close(void *c)
{
	struct tar_cushions_file *tar_c_file = c;

//	if (tar_c_file->bz != NULL) {
//		if (tar_c_file->direction == READ)
//			BZ2_bzReadClose(&tar_c_file->error, tar_c_file->bz);
//		else
//			BZ2_bzWriteClose(&tar_c_file->error, tar_c_file->bz, 0,
//					NULL, NULL);
//	}
//	if (tar_c_file->file != NULL)
//		fclose(tar_c_file->file);
	memset(tar_c_file, 0, sizeof(*tar_c_file));
	free(tar_c_file);

	return 0;
}

static bool mode_is_valid(const struct cushions_handler_mode *mode)
{
	if (mode->append)
		return false;

	if (mode->read && mode->write)
		return false;

	return mode->read || mode->write;
}

static FILE *tar_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct cushions_handler_mode *mode)
{
	int old_errno;
	struct tar_cushions_file *tar_c_file;

	LOGD(__func__);

	if (!mode_is_valid(mode)) {
		errno = EINVAL;
		return NULL;
	}

	tar_c_file = calloc(1, sizeof(*tar_c_file));
	if (tar_c_file == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
//	tar_c_file->file = cushions_fopen(path, mode->mode);
//	if (tar_c_file->file == NULL) {
//		old_errno = errno;
//		LOGPE("cushions_fopen", errno);
//		goto err;
//	}
//	if (mode->read) {
//		tar_c_file->direction = READ;
//		tar_c_file->bz = BZ2_bzReadOpen(&tar_c_file->error,
//				tar_c_file->file, 0, 0, NULL, 0);
//		if (tar_c_file->bz == NULL) {
//			old_errno = EIO;
//			LOGE("BZ2_bzReadOpen error %s(%d)",
//					BZ2_bzerror(tar_c_file->bz,
//							&tar_c_file->error),
//					tar_c_file->error);
//			goto err;
//		}
//	} else {
//		tar_c_file->direction = WRITE;
//		tar_c_file->bz = BZ2_bzWriteOpen(&tar_c_file->error,
//				tar_c_file->file, 9, 0, 0);
//		if (tar_c_file->bz == NULL) {
//			old_errno = EIO;
//			LOGE("BZ2_bzWriteOpen error %s(%d)",
//					BZ2_bzerror(tar_c_file->bz,
//							&tar_c_file->error),
//					tar_c_file->error);
//			goto err;
//		}
//	}

	return fopencookie(tar_c_file, mode->mode,
			tar_cushions_handler.tar_func);
//err:

	tar_close(tar_c_file);

	errno = old_errno;
	return NULL;
}

static ssize_t tar_read(void *c, char *buf, size_t size)
{
	int ret;
//	struct tar_cushions_file *tar_c_file = c;

//	if (tar_c_file->eof)
//		return 0;
//
//	ret = BZ2_bzRead(&tar_c_file->error, tar_c_file->bz, buf, size);
//	if (tar_c_file->error < BZ_OK) {
//		LOGE("BZ2_bzRead error %s(%d)",
//				BZ2_bzerror(tar_c_file->bz, &tar_c_file->error),
//				tar_c_file->error);
//		errno = EIO;
//		return -1;
//	}
//	if (tar_c_file->error == BZ_STREAM_END)
//		tar_c_file->eof = true;

	ret = 0; // TODO
	return ret;
}

static ssize_t tar_write(void *cookie, const char *buf, size_t size)
{
//	struct tar_cushions_file *tar_c_file = cookie;

//	if (tar_c_file->eof)
//		return 0;
//
//	/* buf parameter of BZ2_bzWrite isn't const, don't know why... */
//	BZ2_bzWrite(&tar_c_file->error, tar_c_file->bz, (void *)buf, size);
//	if (tar_c_file->error < BZ_OK) {
//		LOGE("BZ2_bzWrite error %s(%d)",
//				BZ2_bzerror(tar_c_file->bz, &tar_c_file->error),
//				tar_c_file->error);
//		errno = EIO;
//		return 0;
//	}
//	if (tar_c_file->error == BZ_STREAM_END) {
//		tar_c_file->eof = true;
//		return 0;
//	}
//
	return size;
}

static struct tar_cushions_handler tar_cushions_handler = {
	.handler = {
		.name = "tar",
		.fopen = tar_cushions_fopen,
	},
	.tar_func = {
		.read  = tar_read,
		.write = tar_write,
		.close = tar_close
	},
};

static __attribute__((constructor)) void tar_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = cushions_handler_register(&tar_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(tar_cushions_handler): %s",
				strerror(-ret));
}

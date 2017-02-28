#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define CH_LOG_TAG gzip_handler
#include <cushions_handler.h>

#include <zlib.h>

#define BUF_SIZE 0x4000
#define WB_WITH_GZIP_HEADER(wb) (0x10 | (wb))
#define MAX_MEM_LEVEL 9
#define MAX_COMP_LEVEL 9

struct gzip_cushions_handler {
	struct ch_handler handler;
	cookie_io_functions_t gzip_func;
};

enum direction {
	READ,
	WRITE,
};

struct gzip_cushions_file {
	FILE *file;
	unsigned char in[BUF_SIZE];
	unsigned char out[BUF_SIZE];
	struct z_stream_s strm;
	bool strm_initialized;
	bool eof;
	enum direction direction;
};

static struct gzip_cushions_handler gzip_cushions_handler;

static int zerr_to_errno(int err)
{
	switch (err) {
	case Z_ERRNO:
		return errno;

	case Z_STREAM_ERROR:
	case Z_DATA_ERROR:
	case Z_BUF_ERROR:
	case Z_VERSION_ERROR:
		/* codecheck_ignore[USE_NEGATIVE_ERRNO] */
		return EINVAL;

	case Z_MEM_ERROR:
		/* codecheck_ignore[USE_NEGATIVE_ERRNO] */
		return ENOMEM;

	default:
		return 0;
	}
}

static ssize_t gzip_write(void *cookie, const char *buf, size_t size)
{
	size_t sret;
	int ret;
	struct gzip_cushions_file *gzip = cookie;
	size_t remaining;
	struct z_stream_s *strm;
	int flush;

	if (gzip->eof)
		return 0;

	if (size == 0) {
		gzip->eof = true;
		flush = Z_FINISH;
	} else {
		flush = Z_NO_FLUSH;
	}

	strm = &gzip->strm;
	strm->avail_in = size;
	strm->next_in = (unsigned char *)buf;

	do {
		strm->avail_out = BUF_SIZE;
		strm->next_out = gzip->out;
		ret = deflate(strm, flush);
		if (ret == Z_STREAM_ERROR) {
			errno = zerr_to_errno(ret);
			return 0;
		}

		remaining = BUF_SIZE - strm->avail_out;
		sret = fwrite(gzip->out, 1, remaining, gzip->file);
		if (sret != remaining) {
			errno = EIO;
			return 0;
		}
	} while (strm->avail_out == 0);

	return size;
}

static int gzip_close(void *c)
{
	struct gzip_cushions_file *gzip = c;

	if (gzip->strm_initialized) {
		if (gzip->direction == READ) {
			inflateEnd(&gzip->strm);
		} else {
			gzip_write(gzip, "", 0);
			deflateEnd(&gzip->strm);
		}
	}
	if (gzip->file != NULL)
		fclose(gzip->file);
	memset(gzip, 0, sizeof(*gzip));
	free(gzip);

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

static FILE *gzip_cushions_fopen(struct ch_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct ch_mode *mode)
{
	int ret;
	int old_errno;
	struct gzip_cushions_file *gzip;

	LOGD(__func__);

	if (!mode_is_valid(mode)) {
		errno = EINVAL;
		return NULL;
	}
	if (!mode->binary)
		LOGW("binary mode not set, may cause problems on some OSes");

	gzip = calloc(1, sizeof(*gzip));
	if (gzip == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	gzip->file = cushions_fopen(path, mode->mode);
	if (gzip->file == NULL) {
		old_errno = errno;
		LOGPE("cushions_fopen", errno);
		goto err;
	}
	gzip->direction = mode->read ? READ : WRITE;
	if (gzip->direction == READ) {
		ret = inflateInit2(&gzip->strm, WB_WITH_GZIP_HEADER(15));
		if (ret != Z_OK) {
			LOGE("inflateInit2 error");
			old_errno = zerr_to_errno(ret);
			goto err;
		}
	} else {
		ret = deflateInit2(&gzip->strm, MAX_COMP_LEVEL, Z_DEFLATED,
				WB_WITH_GZIP_HEADER(15), MAX_MEM_LEVEL,
				Z_DEFAULT_STRATEGY);
		if (ret != Z_OK) {
			LOGE("deflateInit2 error");
			old_errno = zerr_to_errno(ret);
			goto err;
		}
	}
	gzip->strm_initialized = true;

	return fopencookie(gzip, mode->mode, gzip_cushions_handler.gzip_func);
err:

	gzip_close(gzip);

	errno = old_errno;
	return NULL;
}

static ssize_t gzip_read(void *c, char *buf, size_t size)
{
	errno = ENOSYS;
	return -1;
}

static struct gzip_cushions_handler gzip_cushions_handler = {
	.handler = {
		.name = "gzip",
		.fopen = gzip_cushions_fopen,
	},
	.gzip_func = {
		.read  = gzip_read,
		.write = gzip_write,
		.close = gzip_close
	},
};

static __attribute__((constructor)) void gzip_cushions_handler_constructor(void)
{
	int ret;

	LOGI(__func__);

	ret = ch_handler_register(&gzip_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(gzip_cushions_handler): %s",
				strerror(-ret));
}

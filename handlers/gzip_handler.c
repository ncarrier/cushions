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
	struct gz_header_s header;
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

static int gzip_close(void *c)
{
	struct gzip_cushions_file *gz = c;

	if (gz->strm_initialized) {
		if (gz->direction == READ) {
			// TODO
			inflateEnd(&gz->strm);
		} else {
			gz->strm.avail_in = 0;
			gz->strm.next_in = (unsigned char *) "";
			deflate(&gz->strm, Z_FINISH);
			fwrite(gz->out, 1, BUF_SIZE - gz->strm.avail_out,
					gz->file);
			deflateEnd(&gz->strm);
		}
	}
	if (gz->file != NULL)
		fclose(gz->file);
	memset(gz, 0, sizeof(*gz));
	free(gz);

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
	struct gzip_cushions_file *gz;

	LOGD(__func__);

	if (!mode_is_valid(mode)) {
		errno = EINVAL;
		return NULL;
	}
	if (!mode->binary)
		LOGW("binary mode not set, may cause problems on some OSes");

	gz = calloc(1, sizeof(*gz));
	if (gz == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	gz->file = cushions_fopen(path, mode->mode);
	if (gz->file == NULL) {
		old_errno = errno;
		LOGPE("cushions_fopen", errno);
		goto err;
	}
	gz->direction = mode->read ? READ : WRITE;
	if (gz->direction == READ) {
		ret = inflateInit2(&gz->strm, WB_WITH_GZIP_HEADER(15));
		if (ret != Z_OK) {
			LOGE("inflateInit2 error");
			old_errno = zerr_to_errno(ret);
			goto err;
		}
	} else {
		ret = deflateInit2(&gz->strm, MAX_COMP_LEVEL, Z_DEFLATED,
				WB_WITH_GZIP_HEADER(15), MAX_MEM_LEVEL,
				Z_DEFAULT_STRATEGY);
		if (ret != Z_OK) {
			LOGE("deflateInit2 error");
			old_errno = zerr_to_errno(ret);
			goto err;
		}
		gz->header = (struct gz_header_s){
			.text = false,
			.time = 0,
			.extra = NULL,
			.extra_len = 0,
			.extra_max = 0,
			.name = (Bytef *)"libcushions.so",
			.name_max = 0,
			.comment = (Bytef *)"created with cushions gzip",
			.comm_max = 0,
			.hcrc = true,
			.done = false,
		};
		ret = deflateSetHeader(&gz->strm, &gz->header);
		if (ret != Z_OK) {
			old_errno = zerr_to_errno(ret);
			goto err;
		}
	}
	gz->strm_initialized = true;

	return fopencookie(gz, mode->mode, gzip_cushions_handler.gzip_func);
err:

	gzip_close(gz);

	errno = old_errno;
	return NULL;
}

static ssize_t gzip_read(void *c, char *buf, size_t size)
{
//	int ret;
	struct gzip_cushions_file *gz = c;

	errno = ENOSYS;
	return -1;
	if (gz->eof)
		return 0;

//	ret = my_gzip_read(&gz->gz, buf, size);
//	if (ret < 0)
//		errno = -ret;
//	else if ((size_t)ret < size)
//		gz->eof = true;

//	return ret;
}

static ssize_t gzip_write(void *cookie, const char *buf, size_t size)
{
	size_t sret;
	int ret;
	struct gzip_cushions_file *gz = cookie;
	size_t remaining;
	struct z_stream_s *strm;

	strm = &gz->strm;
	strm->avail_in = size;
	strm->next_in = (unsigned char *)buf;

	do {
		strm->avail_out = BUF_SIZE;
		strm->next_out = gz->out;
		ret = deflate(strm, Z_NO_FLUSH);
		if (ret == Z_STREAM_ERROR) {
			errno = zerr_to_errno(ret);
			return 0;
		}

		remaining = BUF_SIZE - strm->avail_out;
		sret = fwrite(gz->out, 1, remaining, gz->file);
		if (sret != remaining) {
			errno = EIO;
			return 0;
		}
	} while (strm->avail_out == 0);

	return size;
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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include <arpa/inet.h>

#include <lzo/lzo1x.h>

#include <cushions.h>
#include <cushions_handler.h>

#define LOG_TAG lzo_handler
#include "log.h"

#define BUFFER_SIZE 0x400
/* hardcode the mode, we don't have information on the original file */
#define DEFAULT_MODE 0x81ed
#define PLACEHOLDER "placeholder"

#define LZOP_MAGIC_INITIALIZER { 0x89, 0x4c, 0x5a, 0x4f, 0x00, 0x0d, 0x0a, \
	0x1a, 0x0a }

#define LZOP_FLAG_ADLER32_BLOCK 0x1
#define LZOP_FLAG_STDIN 0x4
#define LZOP_FLAG_OS_UNIX 0x3000000

#define DEFAULT_FLAGS (LZOP_FLAG_ADLER32_BLOCK | LZOP_FLAG_STDIN | \
		LZOP_FLAG_OS_UNIX)

#define LZOP_METHOD_LZO1X_1_15 2

struct lzo_cushions_handler {
	struct cushions_handler handler;
	cookie_io_functions_t lzo_func;
};

struct lzo_cushions_file {
	int error;
	FILE *file;
	char buffer[LZO1X_1_MEM_COMPRESS];
	lzo_bytep dest;
	size_t dest_size;
	bool eof;
	/* for a starter, we will use only crc32 checksum */
	uint32_t checksum;
};

/*
 * lzop file header normally has a variable length, but by not supporting the
 * filter flag and by using an empty name, we can simplify the problem back to a
 * fixed length header.
 */
/* all multi-byte fields are big endian (== network order) */
struct lzop_file_header {
	uint16_t version;
	uint16_t lib_version;
	uint16_t version_needed_to_extract;
	uint8_t method;
	uint8_t level;
	uint32_t flags;
	uint32_t mode;
	uint32_t mtime_low;
	uint32_t mtime_high;
	uint8_t name_len;
	uint32_t checksum;
} __attribute__((packed));

struct lzop_block_header {
	uint32_t block_size;
	uint32_t compressed_block_size;
	uint32_t block_checksum;
} __attribute__((packed));

static struct lzo_cushions_handler lzo_cushions_handler;

static int lzo_close(void *c)
{
	struct lzo_cushions_file *lzo_c_file = c;
	uint32_t size = 0;

	/* lzop ends the file with a 0 size */
	fwrite(&size, sizeof(size), 1, lzo_c_file->file);
	if (lzo_c_file->file != NULL)
		fclose(lzo_c_file->file);
	memset(lzo_c_file, 0, sizeof(*lzo_c_file));
	free(lzo_c_file);

	return 0;
}

static void compute_checksum(struct lzop_file_header *h)
{
	/* - 4 because the checksum mustn't be used for the computation */
	h->checksum = htonl(lzo_adler32(1, (void *)h, sizeof(*h) - 4));
}

static int fill_and_write_header(const struct lzo_cushions_file *lzo_c_file)
{
	int ret;
	size_t sret;
	uint64_t t = time(NULL);
	static const uint8_t lzop_magic[9] = { 0x89, 0x4c, 0x5a, 0x4f, 0x00,
			0x0d, 0x0a, 0x1a, 0x0a };
	struct lzop_file_header header = {
			.version = htons(0x1030),
			.lib_version = htons(lzo_version() & 0xffff),
			.version_needed_to_extract = htons(0x0940),
			.method = LZOP_METHOD_LZO1X_1_15,
			.level = 1,
			.flags = htonl(DEFAULT_FLAGS),
			.mode = htonl(DEFAULT_MODE),
			.mtime_low = htonl(t & 0xffffffff),
			.mtime_high = htonl(t >> 32),
			.name_len = 0,
	};

	compute_checksum(&header);

	sret = fwrite(lzop_magic, sizeof(lzop_magic), 1, lzo_c_file->file);
	if (sret != 1) {
		ret = -errno;
		LOGE("%s: fwrite magic: %s", __func__, feof(lzo_c_file->file) ?
						"eof" : strerror(-ret));
		return ret;
	}

	sret = fwrite(&header, sizeof(header), 1, lzo_c_file->file);
	if (sret != 1) {
		ret = -errno;
		LOGE("%s: fwrite: %s", __func__, feof(lzo_c_file->file) ?
						"eof" : strerror(-ret));
		return ret;
	}

	return 0;
}

static FILE *lzo_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct cushions_handler_mode *mode)
{
	int ret;
	int old_errno;
	struct lzo_cushions_file *lzo_c_file;

	LOGD(__func__);

	/* TODO less restrictive condition */
	if (strcmp(mode->mode, "w") != 0 && strcmp(mode->mode, "wb") != 0) {
		LOGE("lzo scheme only supports \"r\" open mode");
		errno = EINVAL;
		return NULL;
	}

	lzo_c_file = calloc(1, sizeof(*lzo_c_file));
	if (lzo_c_file == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	lzo_c_file->file = cushions_fopen(path, mode->mode);
	if (lzo_c_file->file == NULL) {
		old_errno = errno;
		LOGPE("cushions_fopen", errno);
		goto err;
	}

	ret = fill_and_write_header(lzo_c_file);
	if (ret < 0) {
		LOGPE("fill_and_write_header", ret);
		errno = -ret;
		return NULL;
	}

	/* TODO adapt mode according to the mode argument */
	return fopencookie(lzo_c_file, mode->mode,
			lzo_cushions_handler.lzo_func);
err:

	lzo_close(lzo_c_file);

	errno = old_errno;
	return NULL;
}

static int grow_dest_buffer(struct lzo_cushions_file *file, size_t new_size)
{
	void *tmp;
	if (new_size < file->dest_size)
		return 0;

	tmp = realloc(file->dest, new_size);
	if (tmp == NULL)
		return -errno;
	file->dest = tmp;
	file->dest_size = new_size;

	return 0;
}

/**
 *        cookie_write_function_t *write
              This function implements write operations for the stream.
              When called, it receives three arguments:

                  ssize_t write(void *cookie, const char *buf, size_t size);

              The buf and size arguments are, respectively, a buffer of data
              to be output to the stream and the size of that buffer.  As
              its function result, the write function should return the
              number of bytes copied from buf, or 0 on error.  (The function
              must not return a negative value.)  The write function should
              update the stream offset appropriately.

              If *write is a null pointer, then output to the stream is
              discarded.
 */
static ssize_t lzo_write(void *cookie, const char *buf, size_t size)
{
	/*
	 * TODO add an intermediate buffering step to avoid writing many small
	 * blocks
	 */
	int ret;
	size_t uncompressible_data_size;
	struct lzo_cushions_file *f = cookie;
	lzo_uint dst_len;
	size_t sret;
	struct lzop_block_header header;

	/*
	 * from doc/LZO.txt:
	 * When dealing with incompressible data, LZO expands the input block by
	 * a maximum of 64 bytes per 1024 bytes input.
	 */
	uncompressible_data_size = (((size >> 10) + 1) << 6) + size;
	ret = grow_dest_buffer(f, uncompressible_data_size);
	if (ret < 0) {
		LOGPE("grow_dest_buffer", ret);
		errno = -ret;
		return 0;
	}
	/* can't fail */
	lzo1x_1_15_compress((const lzo_bytep)buf, size, f->dest, &dst_len,
			f->buffer);

	header = (struct lzop_block_header) {
		.block_size = htonl(size),
		.compressed_block_size = htonl(dst_len < size ? dst_len : size),
		.block_checksum = htonl(lzo_adler32(1, (uint8_t *)buf, size)),
	};

	sret = fwrite(&header, sizeof(header), 1, f->file);
	if (sret != 1) {
		LOGPE("fwrite header", errno);
		return 0;
	}
	if (dst_len < size) {
		sret = fwrite(f->dest, dst_len, 1, f->file);
		if (sret != 1) {
			LOGPE("fwrite block", errno);
			return 0;
		}
	} else {
		sret = fwrite(buf, size, 1, f->file);
		if (sret != 1) {
			LOGPE("fwrite block", errno);
			return 0;
		}
	}

	return size;
}

static struct lzo_cushions_handler lzo_cushions_handler = {
	.handler = {
		.name = "lzo",
		.fopen = lzo_cushions_fopen,
	},
	.lzo_func = {
		.write = lzo_write,
		.close = lzo_close
	},
};

static __attribute__((constructor)) void lzo_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	lzo_init();

	ret = cushions_handler_register(&lzo_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(lzo_cushions_handler): %s",
				strerror(-ret));
}

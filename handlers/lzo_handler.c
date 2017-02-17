#define _GNU_SOURCE
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#include <arpa/inet.h>

#include <lzo/lzo1x.h>

#define CH_LOG_TAG lzo_handler
#include <cushions_handler.h>

/* hardcode the mode, we don't have information on the original file */
#define DEFAULT_MODE 0x81ed
#define PLACEHOLDER "placeholder"

#define B_SIZE 0x40000
#define MAX_COMPRESSED_SIZE(x)  ((x) + (x) / 16 + 64 + 3)

#define LZOP_MAGIC_INITIALIZER { 0x89, 0x4c, 0x5a, 0x4f, 0x00, 0x0d, 0x0a, \
	0x1a, 0x0a }

#define LZOP_FLAG_ADLER32_BLOCK 0x1
#define LZOP_FLAG_STDIN 0x4
#define LZOP_FLAG_OS_UNIX 0x3000000

#define DEFAULT_FLAGS (LZOP_FLAG_ADLER32_BLOCK | LZOP_FLAG_STDIN | \
		LZOP_FLAG_OS_UNIX)

#define LZOP_METHOD_LZO1X_1_15 2

struct lzo_cushions_handler {
	struct ch_handler handler;
	cookie_io_functions_t lzo_func;
};

struct lzo_cushions_file {
	int error;
	FILE *file;
	char wrkmem[LZO1X_1_MEM_COMPRESS];
	/* where original data will be copied */
	uint8_t src[B_SIZE];
	/* next byte to write in src */
	size_t cur;
	/* where compressed data are stored before being written to file */
	uint8_t dst[MAX_COMPRESSED_SIZE(B_SIZE)];
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

static int write_compressed_block(struct lzo_cushions_file *file)
{
	ssize_t sret;
	lzo_uint dst_len;
	uint32_t final_size;
	uint8_t *final_buf;
	struct lzop_block_header header;

	/* can't fail */
	lzo1x_1_15_compress((const lzo_bytep)file->src, file->cur, file->dst,
			&dst_len,file->wrkmem);

	final_size = dst_len < file->cur ? dst_len : file->cur;
	header = (struct lzop_block_header) {
		.block_size = htonl(file->cur),
		.compressed_block_size = htonl(final_size),
		.block_checksum = htonl(lzo_adler32(1, file->src, file->cur)),
	};

	sret = fwrite(&header, sizeof(header), 1, file->file);
	if (sret != 1) {
		sret = -errno;
		LOGPE("fwrite header", sret);
		return sret;
	}
	final_buf = dst_len < file->cur ? file->dst : file->src;
	sret = fwrite(final_buf, final_size, 1, file->file);
	if (sret != 1) {
		sret = -errno;
		LOGPE("fwrite block", errno);
		return sret;
	}
	file->cur = 0;

	return 0;
}

static int lzo_close(void *c)
{
	int ret;
	int ret_val = 0;
	struct lzo_cushions_file *file = c;
	uint32_t size = 0;
	int old_errno = 0;

	/* write remaining data if needed */
	if (file->cur != 0) {
		ret = write_compressed_block(file);
		if (ret < 0) {
			LOGPE("write_compressed_block", ret);
			old_errno = -ret;
			ret_val = EOF;
		}
	}
	/* lzop ends the file with a 0 size */
	ret = fwrite(&size, sizeof(size), 1, file->file);
	if (ret < 0) {
		LOGPE("fwrite last 0 byte", ret);
		old_errno = -ret;
		ret_val = EOF;
	}
	if (file->file != NULL)
		fclose(file->file);
	memset(file, 0, sizeof(*file));
	free(file);
	errno = old_errno;

	return ret_val;
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

static FILE *lzo_cushions_fopen(struct ch_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct ch_mode *mode)
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

static int store(struct lzo_cushions_file *f, const char *buf, size_t size)
{
	size_t written;
	size_t remaining;

	if (f->cur >= B_SIZE)
		return -ENOMEM;

	remaining = B_SIZE - f->cur;
	written = MIN(size, remaining);
	memcpy(f->src + f->cur, buf, written);
	f->cur += written;

	return written;
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
	int ret;
	size_t consumed = 0;
	struct lzo_cushions_file *file = cookie;

	do {
		ret = store(file, buf + consumed, size - consumed);
		if (ret < 0) {
			LOGPE("store", ret);
			errno = -ret;
			return 0;
		}
		if (file->cur < B_SIZE)
			return size;
		consumed += ret;

		ret = write_compressed_block(file);
		if (ret < 0) {
			LOGPE("write_compressed_block", ret);
			errno = -ret;
			return EOF;
		}
	} while (consumed != size);

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

	ret = ch_handler_register(&lzo_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(lzo_cushions_handler): %s",
				strerror(-ret));
}

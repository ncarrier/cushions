#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include <zlib.h>

#define BUF_SIZE 0x4000

#define WB_WITH_GZIP_HEADER(wb) (0x10 | (wb))

#define MAX_MEM_LEVEL 9
#define MAX_COMP_LEVEL 9

struct gzip {
	struct z_stream_s strm;
	FILE *dest_file;
	unsigned char out[BUF_SIZE];
	bool strm_initialized;
	struct gz_header_s header;
};

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

static void string_cleanup(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

static void file_cleanup(FILE **f)
{
	if (f == NULL || *f == NULL)
		return;
	fclose(*f);

	*f = NULL;
}

static char *build_unzip_dest_path(const char *src_path)
{
	int ret;
	char *res;
	const char *needle;

	needle = strrchr(src_path, '.');
	if (needle == NULL || needle == src_path)
		ret = asprintf(&res, "%s.unzip", src_path);
	else
		ret = asprintf(&res, "%.*s", (int) (needle - src_path),
				src_path);
	if (ret == -1) {
		res = NULL;
		errno = ENOMEM;
	}

	return res;
}

static int gunzip(const char *src_path)
{
	char __attribute__((cleanup(string_cleanup)))*dest_path = NULL;
	FILE __attribute__((cleanup(file_cleanup)))*dest_file = NULL;
	int ret;
	unsigned have;
	unsigned char in[BUF_SIZE];
	unsigned char out[BUF_SIZE];
	size_t sret;
	struct z_stream_s __attribute__((cleanup(inflateEnd))) strm = { 0 };
	FILE __attribute__((cleanup(file_cleanup)))*src_file = NULL;

	src_file = fopen(src_path, "rbe");
	if (src_file == NULL)
		error(EXIT_FAILURE, errno, "fopen");


	dest_path = build_unzip_dest_path(src_path);
	if (dest_path == NULL)
		return -errno;

	dest_file = fopen(dest_path, "wbex");
	if (dest_file == NULL)
		return -errno;

	ret = inflateInit2(&strm, WB_WITH_GZIP_HEADER(15));
	if (ret != Z_OK)
		return -zerr_to_errno(ret);

	do {
		sret = fread(in, 1, BUF_SIZE, src_file);
		if (sret < BUF_SIZE && ferror(src_file))
			return -EIO;
		strm.avail_in = sret;
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;

		do {
			strm.avail_out = BUF_SIZE;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			switch (ret) {
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
			case Z_STREAM_ERROR:
				return -zerr_to_errno(ret);
			}
			have = BUF_SIZE - strm.avail_out;
			sret = fwrite(out, 1, have, dest_file);
			if (sret != have)
				return -EIO;
		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);


	return EXIT_SUCCESS;
}
static ssize_t gzip_fwrite(struct gzip *gzip, const char *buf, size_t size,
		int flush);

static void gzip_fclose(struct gzip **g)
{
	struct gzip *gzip;

	if (g == NULL || *g == NULL)
		return;
	gzip = *g;

	if (gzip->strm_initialized)
		deflateEnd(&gzip->strm);
	file_cleanup(&gzip->dest_file);
	free(gzip);
}

static char *build_zip_dest_path(const char *src_path)
{
	int ret;
	char *res;

	ret = asprintf(&res, "%s.gz", src_path);
	if (ret == -1) {
		res = NULL;
		errno = ENOMEM;
	}

	return res;
}

static struct gzip *gzip_fopen(const char *dest_path)
{
	int ret;
	struct gzip *gzip;
	struct gz_header_s gzip_header = {
		.text = false,
		.time = 0,
		.extra = NULL,
		.extra_len = 0,
		.extra_max = 0,
		.name = (Bytef *)"titi tata tutu",
		.name_max = 0,
		.comment = (Bytef *)"created with cushions gzip",
		.comm_max = 0,
		.hcrc = true,
		.done = false,
	};

	gzip = calloc(1, sizeof(*gzip));
	if (gzip == NULL)
		return NULL;
	gzip->dest_file = fopen(dest_path, "wbex");
	if (gzip->dest_file == NULL) {
		ret = errno;
		goto err;
	}

	ret = deflateInit2(&gzip->strm, MAX_COMP_LEVEL, Z_DEFLATED,
			WB_WITH_GZIP_HEADER(15), MAX_MEM_LEVEL,
			Z_DEFAULT_STRATEGY);
	if (ret != Z_OK) {
		ret = zerr_to_errno(ret);
		goto err;
	}
	gzip->strm_initialized = true;
	gzip->header = gzip_header;
	ret = deflateSetHeader(&gzip->strm, &gzip->header);
	if (ret != Z_OK) {
		ret = zerr_to_errno(ret);
		goto err;
	}

	return gzip;
err:
	gzip_fclose(&gzip);
	errno = ret;

	return NULL;
}

static ssize_t gzip_fwrite(struct gzip *gzip, const char *buf, size_t size,
		int flush)
{
	int ret;
	unsigned have;
	size_t sret;

	gzip->strm.avail_in = size;
	gzip->strm.next_in = (unsigned char *)buf;

	do {
		gzip->strm.avail_out = BUF_SIZE;
		gzip->strm.next_out = gzip->out;
		ret = deflate(&gzip->strm, flush);
		assert(ret != Z_STREAM_ERROR);

		have = BUF_SIZE - gzip->strm.avail_out;
		sret = fwrite(gzip->out, 1, have, gzip->dest_file);
		if (sret != have) {
			errno = EIO;
			return 0;
		}

	} while (gzip->strm.avail_out == 0);
	assert(gzip->strm.avail_in == 0);

	return size;
}

static int gzip(const char *src_path)
{
	int ret;
	int flush = Z_NO_FLUSH;
	size_t sret;
	FILE __attribute__((cleanup(file_cleanup)))*src_file = NULL;
	char __attribute__((cleanup(string_cleanup)))*dest_path = NULL;
	struct gzip __attribute__((cleanup(gzip_fclose)))*gzip = NULL;
	char in[BUF_SIZE];

	src_file = fopen(src_path, "rbe");
	if (src_file == NULL)
		error(EXIT_FAILURE, errno, "fopen");

	dest_path = build_zip_dest_path(src_path);
	if (dest_path == NULL)
		return -errno;

	gzip = gzip_fopen(dest_path);
	if (gzip == NULL) {
		ret = -errno;
		perror("gzip_fopen");
		return ret;
	}

	do {
		sret = fread(in, 1, BUF_SIZE, src_file);
		if (sret < BUF_SIZE) {
			if (ferror(src_file))
				return -EIO;
			flush = Z_FINISH;
		}
		sret = gzip_fwrite(gzip, in, sret, flush);
		if (sret == 0)
			return -errno;
	} while (flush != Z_FINISH);

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	const char *progname;
	bool compress;
	const char *path;

	progname = basename(argv[0]);
	compress = strcmp(progname, "gzip") == 0;
	if (argc != 2)
		error(EXIT_FAILURE, 0, "usage: %s file", progname);
	path = argv[1];

	ret = (compress ? gzip : gunzip)(path);
	if (ret < 0)
		error(EXIT_FAILURE, -ret, "%s", progname);

	return EXIT_SUCCESS;
}

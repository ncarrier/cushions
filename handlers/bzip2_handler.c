#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <cushion_handler.h>

#define BCH_LOG(level, ...) cushion_handler_log( \
		&bzip2_cushion_handler.handler, (level), __VA_ARGS__)
#define BCH_LOGW(...) BCH_LOG(CUSHION_HANDLER_WARNING, __VA_ARGS__)
#define BCH_LOGI(...) BCH_LOG(CUSHION_HANDLER_INFO, __VA_ARGS__)
#define BCH_LOGD(...) BCH_LOG(CUSHION_HANDLER_DEBUG, __VA_ARGS__)

struct bzip2_cushion_handler {
	struct cushion_handler handler;
	cookie_io_functions_t bzip2_func;
};

static const struct bzip2_cushion_handler bzip2_cushion_handler;

static FILE *bzip2_cushion_fopen(struct cushion_handler *handler,
		const char *path, const char *mode, const char *envz,
		size_t envz_len)
{
	BCH_LOGD(__func__);

	/* TODO here */

	/* TODO adapt mode according to the mode argument */
	return fopencookie((void *)&bzip2_cushion_handler, "r",
			bzip2_cushion_handler.bzip2_func);
}

static ssize_t bzip2_write(void *c, const char *buf, size_t size)
{

	errno = ENOSYS;
	return -1;
}

static ssize_t bzip2_read(void *c, char *buf, size_t size)
{

	errno = ENOSYS;
	return -1;
}

static int bzip2_seek(void *c, off64_t *offset, int whence)
{

	errno = ENOSYS;
	return -1;
}

static int bzip2_close(void *c)
{

	errno = ENOSYS;
	return -1;
}

static const struct bzip2_cushion_handler bzip2_cushion_handler = {
	.handler = {
		.scheme = "bzip2",
		.fopen = bzip2_cushion_fopen,
	},
	.bzip2_func = {
		.read  = bzip2_read,
		.write = bzip2_write,
		.seek  = bzip2_seek,
		.close = bzip2_close
	},
};

__attribute__((constructor)) void bzip2_cushion_handler_constructor(void)
{
	int ret;

	BCH_LOGI(__func__);

	ret = cushion_handler_register(&bzip2_cushion_handler.handler);
	if (ret < 0)
		BCH_LOGW("cushion_handler_register(bzip2_cushion_handler): %s",
				strerror(-ret));
}

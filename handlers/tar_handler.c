#define _GNU_SOURCE
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "tar.h"

#define CH_LOG_TAG tar_handler
#include <cushions_handler.h>

struct tar_cushions_handler {
	struct ch_handler handler;
	cookie_io_functions_t tar_func;
};

static struct tar_cushions_handler tar_cushions_handler;

static int tar_handler_close(void *c)
{
	struct tar *tar = c;

	tar_cleanup(tar);

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

static FILE *tar_cushions_fopen(const struct ch_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct ch_mode *mode)
{
	int ret;
	int old_errno;
	struct tar *tar;

	LOGD(__func__);

	if (!mode_is_valid(mode)) {
		errno = EINVAL;
		return NULL;
	}

	tar = calloc(1, sizeof(*tar));
	if (tar == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	ret = tar_init(tar, path, mode->read ? TAR_READ : TAR_WRITE);
	if (ret < 0) {
		old_errno = -ret;
		LOGPE("tar_init", ret);
		goto err;
	}

	return fopencookie(tar, mode->mode, tar_cushions_handler.tar_func);
err:

	tar_cleanup(tar);

	errno = old_errno;
	return NULL;
}

static ssize_t tar_handler_read(void *cookie, char *buf, size_t size)
{
	struct tar *tar = cookie;

	if (!tar_is_direction_read(tar)) {
		errno = EINVAL;
		return -1;
	}

	return tar_read(tar, buf, size);
}

static ssize_t tar_handler_write(void *cookie, const char *buf, size_t size)
{
	struct tar *tar = cookie;

	if (tar_is_direction_read(tar)) {
		errno = EINVAL;
		return 0;
	}

	return tar_write(tar, buf, size);
}

static struct tar_cushions_handler tar_cushions_handler = {
	.handler = {
		.name = "tar",
		.fopen = tar_cushions_fopen,
	},
	.tar_func = {
		.read  = tar_handler_read,
		.write = tar_handler_write,
		.close = tar_handler_close
	},
};

static __attribute__((constructor)) void tar_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = ch_handler_register(&tar_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(tar_cushions_handler): %s",
				strerror(-ret));
}

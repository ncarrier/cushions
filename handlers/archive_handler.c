#define _GNU_SOURCE
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <archive.h>

#define CH_LOG_TAG archive_handler
#include <cushions_handler.h>

struct archive_cushions_handler {
	struct ch_handler handler;
	cookie_io_functions_t archive_func;
};

struct arch {
	int dummy;
};

static struct archive_cushions_handler archive_cushions_handler;

static int archive_handler_close(void *c)
{
	struct arch *arch = c;

	free(arch);

	return 0;
}

static bool mode_is_valid(const struct ch_mode *mode)
{
	if (mode->append)
		return false;

	if (mode->read && mode->write)
		return false;

	return mode->write;
}

static FILE *archive_cushions_fopen(struct ch_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct ch_mode *mode)
{
	int old_errno;
	struct arch *arch;

	LOGD(__func__);

	if (!mode_is_valid(mode)) {
		errno = EINVAL;
		return NULL;
	}

	arch = calloc(1, sizeof(*arch));
	if (arch == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		goto err;
	}

	return fopencookie(arch, mode->mode,
			archive_cushions_handler.archive_func);
err:

	archive_handler_close(arch);

	errno = old_errno;
	return NULL;
}

static ssize_t archive_handler_read(void *cookie, char *buf, size_t size)
{
	errno = ENOSYS;

	return -1;
}

static ssize_t archive_handler_write(void *cookie, const char *buf, size_t size)
{
	errno = ENOSYS;

	return -1;
}

static struct archive_cushions_handler archive_cushions_handler = {
	.handler = {
		.name = "archive",
		.fopen = archive_cushions_fopen,
	},
	.archive_func = {
		.read  = archive_handler_read,
		.write = archive_handler_write,
		.close = archive_handler_close
	},
};

static __attribute__((constructor)) void archive_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = ch_handler_register(&archive_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(archive_cushions_handler): %s",
				strerror(-ret));
}

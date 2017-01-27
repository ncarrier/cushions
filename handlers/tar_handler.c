#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "tar.h"

#define LOG_TAG tar_handler
#include <cushions_handler.h>

struct tar_cushions_handler {
	struct cushions_handler handler;
	cookie_io_functions_t tar_func;
};

enum direction {
	READ,
	WRITE,
};

struct tar_cushions_file {
	union {
		struct tar_out out;
	};
	enum direction direction;
	bool eof;
};

static struct tar_cushions_handler tar_cushions_handler;

static int tar_close(void *c)
{
	struct tar_cushions_file *tar_c_file = c;

	if (tar_c_file->direction == WRITE)
		tar_c_file->out.o.cleanup(&tar_c_file->out);
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
	int ret;
	int old_errno;
	struct tar_cushions_file *tar_c_file;

	LOGD(__func__);

	if (!mode_is_valid(mode)) {
		errno = EINVAL;
		return NULL;
	}

	// TODO use path as the directory destination
	tar_c_file = calloc(1, sizeof(*tar_c_file));
	if (tar_c_file == NULL) {
		old_errno = errno;
		LOGPE("calloc", errno);
		errno = old_errno;
		return NULL;
	}
	if (mode->read) {
		tar_c_file->direction = READ;
		old_errno = ENOSYS;
		LOGE("reading from directory to tar archive not supported yet");
		goto err;
	} else {
		tar_c_file->direction = WRITE;
		ret = tar_out_init(&tar_c_file->out, path);
		if (ret < 0) {
			old_errno = ret;
			LOGPE("tar_out_init", ret);
			goto err;
		}
	}

	return fopencookie(tar_c_file, mode->mode,
			tar_cushions_handler.tar_func);
err:

	tar_close(tar_c_file);

	errno = old_errno;
	return NULL;
}

//static ssize_t tar_read(void *c, char *buf, size_t size)
//{
//	int ret;
//
//	return ret;
//}

static ssize_t tar_write(void *cookie, const char *buf, size_t size)
{
	struct tar_cushions_file *tar_c_file = cookie;
	unsigned consumed;
	unsigned total_consumed;
	struct tar_out *to = &tar_c_file->out;
	int ret;
	const char *p;

	if (tar_c_file->eof)
		return 0;

	total_consumed = 0;
	p = buf;
	do {
		consumed = to->o.store_data(to, p, size);
		if (to->o.is_full(to)) {
			ret = to->o.process_block(to);
			if (ret < 0) {
				errno = -ret;
				return 0;
			}
			if (ret == TAR_OUT_END) {
				tar_c_file->eof = true;
				break;
			}
		}
		p += consumed;
		total_consumed += consumed;
		size -= consumed;
	} while (size > 0);

	return total_consumed;
}

static struct tar_cushions_handler tar_cushions_handler = {
	.handler = {
		.name = "tar",
		.fopen = tar_cushions_fopen,
	},
	.tar_func = {
//		.read  = tar_read,
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

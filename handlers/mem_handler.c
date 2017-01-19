#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>

#include <cushions.h>
#include <cushions_handler.h>

#define LOG_TAG mem_handler
#include "log.h"

struct mem_cushions_handler {
	struct cushions_handler handler;
	cookie_io_functions_t mem_func;
};

struct mem_cushions_file {
	FILE *file;
	char *buffer;
	size_t size;
};

static FILE *mem_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct cushions_handler_mode *mode)
{
	errno = -ENOSYS;

//	FILE *open_memstream(char **ptr, size_t *sizeloc);
	return NULL;
}

static int mem_close(void *c)
{
	errno = -ENOSYS;

	return 0;
}

static ssize_t mem_write(void *cookie, const char *buf, size_t size)
{
	errno = -ENOSYS;

	return 0;
}

static struct mem_cushions_handler mem_cushions_handler = {
	.handler = {
		.name = "mem",
		.fopen = mem_cushions_fopen,
	},
	.mem_func = {
		.write = mem_write,
		.close = mem_close
	},
};

static __attribute__((constructor)) void lzo_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = cushions_handler_register(&mem_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(mem_cushions_handler): %s",
				strerror(-ret));
}

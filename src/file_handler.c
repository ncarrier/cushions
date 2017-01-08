#include <stdlib.h>

#include <cushions_handler.h>

#define LOG_TAG file_handler
#include "log.h"

struct file_cushions_handler {
	struct cushions_handler handler;
	/* here could come some custom data */
};

static FILE *file_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *mode)
{
	return cushions_handler_real_fopen(path, mode);
}

static const struct file_cushions_handler file_cushions_handler = {
	.handler = {
		.scheme = "file",
		.fopen = file_cushions_fopen,
	},
};

static __attribute__((constructor)) void file_cushions_handler_constructor(void)
{
	int ret;

	ret = cushions_handler_register(&file_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(file_cushions_handler) "
				"failed\n");
}

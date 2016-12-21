#include <stdlib.h>

#include <cushion_handler.h>

#define LOG_TAG file_handler
#include "log.h"

struct file_cushion_handler {
	struct cushion_handler handler;
	/* here could come some custom data */
};

static FILE *file_cushion_fopen(struct cushion_handler *handler,
		const char *path, const char *mode)
{
	return cushion_handler_real_fopen(path, mode);
}

static const struct file_cushion_handler file_cushion_handler = {
	.handler = {
		.scheme = "file",
		.fopen = file_cushion_fopen,
	},
};

__attribute__((constructor)) void file_cushion_handler_constructor(void)
{
	int ret;

	ret = cushion_handler_register(&file_cushion_handler.handler);
	if (ret < 0)
		LOGW("cushion_handler_register(file_cushion_handler) "
				"failed\n");
}

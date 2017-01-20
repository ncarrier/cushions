#include <stdlib.h>

#define LOG_TAG file_handler
#include <cushions_handler.h>

struct file_cushions_handler {
	struct cushions_handler handler;
	/* here could come some custom data */
};

static FILE *file_cushions_fopen(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct cushions_handler_mode *mode)
{
	return cushions_fopen(path, mode->mode);
}

static struct file_cushions_handler file_cushions_handler = {
	.handler = {
		.name = "file",
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

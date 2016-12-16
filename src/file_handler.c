#include <stdlib.h>

#include <cushion_handler.h>

#define FCH_LOG(level, ...) cushion_handler_log( \
		&file_cushion_handler.handler, (level), __VA_ARGS__)
#define FCH_LOGW(...) FCH_LOG(CUSHION_HANDLER_WARNING, __VA_ARGS__)

struct file_cushion_handler {
	struct cushion_handler handler;
	/* here could come some custom data */
};

static FILE *file_cushion_fopen(struct cushion_handler *handler,
		const char *path, const char *mode, const char *envz,
		size_t envz_len)
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
		FCH_LOGW("cushion_handler_register(file_cushion_handler) "
				"failed\n");
}

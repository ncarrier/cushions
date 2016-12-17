#include <stdlib.h>
#include <string.h>

#include <cushion_handler.h>

#define BCH_LOG(level, ...) cushion_handler_log( \
		&bzip2_cushion_handler.handler, (level), __VA_ARGS__)
#define BCH_LOGW(...) BCH_LOG(CUSHION_HANDLER_WARNING, __VA_ARGS__)
#define BCH_LOGI(...) BCH_LOG(CUSHION_HANDLER_INFO, __VA_ARGS__)
#define BCH_LOGD(...) BCH_LOG(CUSHION_HANDLER_DEBUG, __VA_ARGS__)

struct bzip2_cushion_handler {
	struct cushion_handler handler;
	/* here could come some custom data */
};

static const struct bzip2_cushion_handler bzip2_cushion_handler;

static FILE *bzip2_cushion_fopen(struct cushion_handler *handler,
		const char *path, const char *mode, const char *envz,
		size_t envz_len)
{
	BCH_LOGD(__func__);

	/* TODO here */

	return cushion_handler_real_fopen(path, mode);
}

static const struct bzip2_cushion_handler bzip2_cushion_handler = {
	.handler = {
		.scheme = "bzip2",
		.fopen = bzip2_cushion_fopen,
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

#include <stdlib.h>

#include <cushion_handler.h>

#define NCH_LOG(level, ...) cushion_handler_log( \
		&noop_cushion_handler.handler, (level), __VA_ARGS__)
#define NCH_LOGE(...) NCH_LOG(CUSHION_HANDLER_ERROR, __VA_ARGS__)
#define NCH_LOGW(...) NCH_LOG(CUSHION_HANDLER_WARNING, __VA_ARGS__)
#define NCH_LOGI(...) NCH_LOG(CUSHION_HANDLER_INFO, __VA_ARGS__)
#define NCH_LOGD(...) NCH_LOG(CUSHION_HANDLER_DEBUG, __VA_ARGS__)

struct noop_cushion_handler {
	struct cushion_handler handler;
	/* here could come some custom data */
};

static FILE *noop_cushion_fopen(struct cushion_handler *handler,
		const char *path, const char *mode, const char *envz,
		size_t envz_len)
{
	return cushion_handler_real_fopen(path, mode);
}

static const struct noop_cushion_handler noop_cushion_handler = {
	.handler = {
		.scheme = "noop",
		.fopen = noop_cushion_fopen,
	},
};

__attribute__((constructor)) void noop_cushion_handler_constructor(void)
{
	int ret;

	ret = cushion_handler_register(&noop_cushion_handler.handler);
	if (ret < 0)
		NCH_LOGW("cushion_handler_register(noop_cushion_handler) "
				"failed\n");
}

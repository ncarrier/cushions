#ifndef _CUSHION_HANDLER_H
#define _CUSHION_HANDLER_H
#include <stdio.h>

struct cushion_handler;

typedef FILE *(*cushion_handler_fopen)(struct cushion_handler *handler,
		const char *path, const char *mode, const char *envz,
		size_t envz_len);

struct cushion_handler {
	const char *scheme;
	cushion_handler_fopen fopen;

	/* data private to the cushion library */
	const struct cushion_handler *self;
};

#define CUSHION_HANDLER_ERROR 0
#define CUSHION_HANDLER_WARNING 1
#define CUSHION_HANDLER_INFO 2
#define CUSHION_HANDLER_DEBUG 3

void cushion_handler_log(const struct cushion_handler *handler, int level,
		const char *fmt, ...);
int cushion_handler_register(const struct cushion_handler *handler);
FILE *cushion_handler_real_fopen(const char *path, const char *mode);

#endif /* _CUSHION_HANDLER */

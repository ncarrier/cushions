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

int cushion_handler_register(const struct cushion_handler *handler);
FILE *cushion_handler_real_fopen(const char *path, const char *mode);

#endif /* _CUSHION_HANDLER */

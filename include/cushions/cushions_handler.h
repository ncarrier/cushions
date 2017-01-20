#ifndef _CUSHIONS_HANDLER_H
#define _CUSHIONS_HANDLER_H
#include <stdio.h>
#include <stdbool.h>

#include <cushions.h>
#include <cushions_handler_dict.h>
#include <cushions_handler_log.h>
#include <cushions_handler_utils.h>
#include <cushions_handler_mode.h>

struct cushions_handler;

typedef FILE *(*cushions_handler_fopen)(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *name,
		const struct cushions_handler_mode *mode);

typedef bool (*cushions_handler_handles)(struct cushions_handler *handler,
		const char *name);

struct cushions_handler {
	const char *name;
	cushions_handler_fopen fopen;
	/* optional, if null, name will be compared against the url's scheme */
	cushions_handler_handles handles;
};

int cushions_handler_register( struct cushions_handler *handler);
FILE *cushions_handler_real_fopen(const char *path, const char *mode);
int cushions_handler_break_params(const char *input, char **path,
		char **envz, size_t *envz_len);

#endif /* _CUSHIONS_HANDLER */

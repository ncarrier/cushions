#ifndef _CUSHIONS_HANDLER_H
#define _CUSHIONS_HANDLER_H
#include <stdio.h>
#include <stdbool.h>

#include <cushions.h>
#include <cushions_handler_dict.h>
#include <cushions_handler_log.h>
#include <cushions_handler_utils.h>
#include <cushions_handler_mode.h>

struct ch_handler;

typedef FILE *(*ch_fopen)(struct ch_handler *handler, const char *path,
		const char *full_path, const char *name,
		const struct ch_mode *mode);

typedef bool (*ch_handler_handles)(struct ch_handler *handler,
		const char *name);

struct ch_handler {
	const char *name;
	ch_fopen fopen;
	/* optional, if null, name will be compared against the url's scheme */
	ch_handler_handles handles;
};

CUSHIONS_API int ch_handler_register(struct ch_handler *handler);
CUSHIONS_API FILE *ch_handler_real_fopen(const char *path, const char *mode);
CUSHIONS_API int ch_break_params(const char *input, char **path, char **envz,
		size_t *envz_len);

#endif /* _CUSHIONS_HANDLER */

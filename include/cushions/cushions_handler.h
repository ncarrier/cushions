#ifndef _CUSHIONS_HANDLER_H
#define _CUSHIONS_HANDLER_H
#include <stdio.h>

struct cushions_handler;

/* TODO use broken up mode as the last argument */
typedef FILE *(*cushions_handler_fopen)(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const char *mode);

int cushions_handler_break_params(const char *input, char **path,
		char **envz, size_t *envz_len);

struct cushions_handler {
	const char *scheme;
	cushions_handler_fopen fopen;

	/* data private to the cushion library */
	const struct cushions_handler *self;
};

int cushions_handler_register(const struct cushions_handler *handler);
FILE *cushions_handler_real_fopen(const char *path, const char *mode);

struct cushions_handler_mode {
	int read:1;
	int beginning:1;
	int end:1;
	int write:1;
	int truncate:1;
	int create:1;
	int binary:1;
	int append:1;
	int no_cancellation:1;
	int cloexec:1;
	int mmap:1;
	int excl:1;
	char *ccs;
};

int cushions_handler_mode_from_string(struct cushions_handler_mode *mode,
		const char *str);
int cushions_handler_mode_to_string(const struct cushions_handler_mode *mode,
		char **str);
/* must be called after mode_fnom_string to free resources */
void cushions_handler_mode_cleanup(struct cushions_handler_mode *mode);
/* for debug only */
void cushions_handler_mode_dump(const struct cushions_handler_mode *mode);

#endif /* _CUSHIONS_HANDLER */

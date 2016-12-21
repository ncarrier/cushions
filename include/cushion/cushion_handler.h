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

struct cushion_handler_mode {
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

int cushion_handler_mode_from_string(struct cushion_handler_mode *mode,
		const char *str);
int cushion_handler_mode_to_string(const struct cushion_handler_mode *mode,
		char **str);
/* must be called after mode_fnom_string to free resources */
void cushion_handler_mode_cleanup(struct cushion_handler_mode *mode);
/* for debug only */
void cushion_handler_mode_dump(const struct cushion_handler_mode *mode);

#endif /* _CUSHION_HANDLER */

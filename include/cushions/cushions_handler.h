#ifndef _CUSHIONS_HANDLER_H
#define _CUSHIONS_HANDLER_H
#include <stdio.h>
#include <stdbool.h>

#include <cushions.h>
#include <log.h>
#include <utils.h>
#include <dict.h>

struct cushions_handler;

struct cushions_handler_mode {
	bool read:1;
	bool beginning:1;
	bool end:1;
	bool write:1;
	bool truncate:1;
	bool create:1;
	bool binary:1;
	bool append:1;
	bool no_cancellation:1;
	bool cloexec:1;
	bool mmap:1;
	bool excl:1;
	/* original mode string, if any */
	char *mode;
	char *ccs;
};

typedef FILE *(*cushions_handler_fopen)(struct cushions_handler *handler,
		const char *path, const char *full_path, const char *name,
		const struct cushions_handler_mode *mode);

typedef bool (*cushions_handler_handles)(struct cushions_handler *handler,
		const char *name);

int cushions_handler_break_params(const char *input, char **path,
		char **envz, size_t *envz_len);

struct cushions_handler {
	const char *name;
	cushions_handler_fopen fopen;
	/* optional, if null, name will be compared against the url's scheme */
	cushions_handler_handles handles;
};

int cushions_handler_register( struct cushions_handler *handler);
FILE *cushions_handler_real_fopen(const char *path, const char *mode);

int cushions_handler_mode_from_string(struct cushions_handler_mode *mode,
		const char *str);
int cushions_handler_mode_to_string(const struct cushions_handler_mode *mode,
		char **str);
/* must be called after mode_fnom_string to free resources */
void cushions_handler_mode_cleanup(struct cushions_handler_mode *mode);
/* for debug only */
void cushions_handler_mode_dump(const struct cushions_handler_mode *mode);

#endif /* _CUSHIONS_HANDLER */

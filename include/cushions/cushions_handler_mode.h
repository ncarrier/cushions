/*
 * one should not need to include this header directly, include
 * cushions_handler.h instead
 */
#ifndef CUSHIONS_HANDLER_MODE_H_
#define CUSHIONS_HANDLER_MODE_H_
#include <stdbool.h>

#include <cushions_common.h>

struct ch_mode {
	bool read:1;
	bool beginning:1;
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

CUSHIONS_API int ch_mode_from_string(struct ch_mode *mode, const char *str);
CUSHIONS_API int ch_mode_to_string(const struct ch_mode *mode, char **str);
/* must be called after mode_fnom_string to free resources */
CUSHIONS_API void ch_mode_cleanup(struct ch_mode *mode);
/* for debug only */
CUSHIONS_API void ch_mode_dump(const struct ch_mode *mode);

#endif /* CUSHIONS_HANDLER_MODE_H_ */

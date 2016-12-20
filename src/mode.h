#ifndef _MODE_H
#define _MODE_H

struct mode {
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

int mode_from_string(struct mode *mode, const char *str);
int mode_to_string(const struct mode *mode, char **str);
/* must be called after mode_fnom_string to free resources */
void mode_cleanup(struct mode *mode);
/* for debug only */
void mode_dump(const struct mode *mode);

#endif /* _MODE_H */

#ifndef HANDLERS_TAR_H_
#define HANDLERS_TAR_H_
#include <sys/stat.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#include <libtar.h>

#include "coro.h"

enum tar_direction {
	TAR_READ,
	TAR_WRITE,
};

struct tar {
	TAR *tar;
	struct coro_context coro;
	struct coro_stack stack;
	/* parent (root) coroutine */
	struct coro_context parent;
	char *path;

	/* current read informations */
	void *buf;
	size_t size;
	size_t cur;
	tartype_t ops;

	int err;
	bool eof;
};

/*
 * direction must be TAR_WRITE for reading from a tar archive to a tree
 * structure and TAR_READ for reading from a tree structure to a tar archive
 * (i.e. it must be choosen from the tree structure's point of view)
 */
int tar_init(struct tar *tar, const char *path, enum tar_direction direction);

/* returns true iif TAR_WRITE was passed to tar_init */
bool tar_is_direction_read(const struct tar *tar);

ssize_t tar_write(struct tar *tar, const void *buf, size_t size);

ssize_t tar_read(struct tar *tar, void *buf, size_t size);

void tar_cleanup(struct tar *tar);

#endif /* HANDLERS_TAR_H_ */

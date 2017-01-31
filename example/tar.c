#define _GNU_SOURCE
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <error.h>

#include <libtar.h>

#include "picoro.h"

#define BUFFER_SIZE 500

#define TAR_IN_DEFAULT_MODE 0644

#define RESUME_CONTINUE 1
#define RESUME_FULLFILLED 2
#define RESUME_ENDED 3

struct tar_in {
	TAR *tar;
	coro c;
	char *src;
	/* simply linked list at first, may be more elaborate later */
	struct tar_in *next;
};

struct tar_in_coroutine_arg {
	void *buf;
	size_t size;
	size_t cur;
	struct tar_in *ti;
};

static void string_cleanup(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

static int func_tar_in_open(const char *path, int flags, ...)
{
	/*
	 * "opening" operation have already occurred before and the return
	 * value is not significant
	 */
	printf("%s(%s, %d) -> 0\n", __func__, path, flags);

	return 0;
}

static int func_tar_in_close(int fd)
{
	ssize_t sret = RESUME_ENDED;

	yield(&sret);

	printf("%s(%d) -> 0\n", __func__, fd);

	return 0;
}

static ssize_t func_tar_in_read(int fd, void *buf, size_t size)
{
	assert(false);
	return 0;
}

static ssize_t func_tar_in_write(int fd, const void *buf, size_t size)
{
	ssize_t sret;
	struct tar_in_coroutine_arg *arg;
	unsigned consumed;
	unsigned needed;
	size_t remaining;

	remaining = size;
	do {
		sret = RESUME_CONTINUE;
		arg = yield(&sret);
		needed = arg->size - arg->cur;
		consumed = MIN(needed, remaining);
		memcpy((char *)arg->buf + arg->cur, buf, consumed);
		arg->cur += consumed;
		remaining -= consumed;
		buf = ((char *)buf + consumed);
		if (arg->cur == arg->size) {
			sret = RESUME_FULLFILLED;
			arg = yield(&sret);
		}
	} while (remaining != 0);

	printf("%s(%d, %p, %zu) -> %zd\n", __func__, fd, buf, size, sret);

	return size;
}

tartype_t tar_in_ops = {
	.openfunc = func_tar_in_open,
	.closefunc = func_tar_in_close,
	.readfunc = func_tar_in_read,
	.writefunc = func_tar_in_write,
};

static ssize_t tar_in_read(struct tar_in *ti, void *buf, size_t size)
{
	struct tar_in_coroutine_arg arg = {
			.buf = buf,
			.size = size,
			.ti = ti,
			.cur = 0,
	};
	ssize_t *sret;

	do {
		sret = resume(ti->c, &arg);
		if (sret == NULL)
			return errno == 0 ? 0 : -1;
	} while (*sret == RESUME_CONTINUE);

	/* make func_tar_in_close return */
	if (*sret == RESUME_ENDED)
		resume(ti->c, NULL);

	printf("%s(%p, %zu) -> %zu\n", __func__, arg.buf, size, arg.cur);

	return arg.cur;
}

static void *tar_in_serialize(void *arg)
{
	struct tar_in_coroutine_arg *a = arg;
	struct tar_in *ti = a->ti;
	int ret;

	ret = tar_append_tree(ti->tar, ti->src, basename(ti->src));
	if (ret < 0)
		return NULL;
	ret = tar_append_eof(ti->tar);
	if (ret < 0)
		return NULL;
	ret = tar_close(ti->tar);
	if (ret < 0)
		return NULL;

	errno = 0;

	return NULL;
}

static int tar_in_init(struct tar_in *ti, const char *src)
{
	int ret;

	memset(ti, 0, sizeof(*ti));

	ti->src = strdup(src);
	if (ti->src == NULL)
		return -errno;
	ret = tar_open(&ti->tar, src, &tar_in_ops, O_WRONLY | O_CREAT,
			TAR_IN_DEFAULT_MODE,
			TAR_GNU);
	if (ret < 0) {
		string_cleanup(&ti->src);
		return -errno;
	}
	ti->c = coroutine(tar_in_serialize);

	return 0;
}

static void tar_in_cleanup(struct tar_in *ti)
{
	string_cleanup(&ti->src);
}

static struct tar_in ti;
static FILE *dest;

static void cleanup(void)
{
	if (dest != NULL)
		fclose(dest);

	dest = NULL;

	tar_in_cleanup(&ti);
}

int main(int argc, char *argv[])
{
	int ret;
	const char *src_path;
	const char *dest_path;
	ssize_t sread;
	char buf[BUFFER_SIZE];
	bool eof = false;
	ssize_t swritten;

	if (argc < 3)
		error(EXIT_FAILURE, 0, "usage: tar archive_name directory");
	dest_path = argv[1];
	src_path = argv[2];

	ret = tar_in_init(&ti, src_path);
	if (ret < 0)
		error(EXIT_FAILURE, -ret, "tar_in_init");
	dest = fopen(dest_path, "wbe");
	if (dest == NULL)
		error(EXIT_FAILURE, errno, "fopen(%s)", dest_path);
	atexit(cleanup);

	do {
		sread = tar_in_read(&ti, buf, BUFFER_SIZE);
		if (sread != BUFFER_SIZE) {
			if (sread < 0)
				error(EXIT_FAILURE, -sread, "tar_in_read");
			else
				eof = true;
		}
		swritten = fwrite(buf, 1, sread, dest);
		if (swritten != sread) {
			if (ferror(dest))
				error(EXIT_FAILURE, errno, "fwrite");
			else
				error(EXIT_FAILURE, 0, "EOF on dest");
		}
	} while (!eof);

	return EXIT_SUCCESS;
}

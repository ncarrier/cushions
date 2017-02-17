#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <callback.h>

#include "tar.h"

#define DEFAULT_MODE 0644

#if __SIZEOF_INT__ == __SIZEOF_SIZE_T__
#define va_arg_size_t va_arg_uint
#define va_start_ssize_t va_start_int
#define va_return_ssize_t va_return_int
#elif __SIZEOF_LONG__ == __SIZEOF_SIZE_T__
#define va_arg_size_t va_arg_ulong
#define va_start_ssize_t va_start_long
#define va_return_ssize_t va_return_long
#elif __SIZEOF_LONG_LONG__ == __SIZEOF_SIZE_T__
#define va_arg_size_t va_arg_ulonglong
#define va_start_ssize_t va_start_longlong
#define va_return_ssize_t va_return_longlong
#endif

static void ch_string_cleanup(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

static void transfer_to_parent(struct tar *tar)
{
	/* not very useful in itself, but silences the "clobbered" warnings */
	coro_transfer(&tar->coro, &tar->parent);
}

static ssize_t func_tar_read(int fd, void *buf, size_t size)
{
	assert(false);
	return 0;
}

static ssize_t func_tar_write(int fd, const void *buf, size_t size)
{
	assert(false);
	return 0;
}

static int func_tar_open(const char *path, int flags, ...)
{
	return 0;
}

static int func_tar_close(int fd)
{
	return 0;
}

/*
 * used for partial initialization only, default behavior for I/O is to assert,
 * only one direction function is overwritten in constructor
 */
static tartype_t tar_ops = {
		.readfunc = func_tar_read,
		.writefunc = func_tar_write,

		.openfunc = func_tar_open,
		.closefunc = func_tar_close,
};

static bool is_tar_in(const struct tar *tar)
{
	return tar->ops.writefunc != func_tar_write;
}

static ssize_t do_tar_read_write(struct tar *tar, int fd, void *buf,
		size_t size)
{
	unsigned consumed;
	unsigned rest;
	volatile size_t waited;

	waited = size;
	do {
		rest = tar->size - tar->cur;
		consumed = MIN(rest, waited);
		if (is_tar_in(tar))
			memcpy((char *)tar->buf + tar->cur, buf, consumed);
		else
			memcpy(buf, (char *)tar->buf + tar->cur, consumed);
		tar->cur += consumed;
		waited -= consumed;
		buf = (char *)buf + consumed;
		if (tar->cur == tar->size)
			transfer_to_parent(tar);
	} while (waited != 0);

	return size;
}

static void tar_callback(void *tar, va_alist alist)
{
	int fd;
	void *buf;
	size_t size;

	ssize_t sret;

	va_start_ssize_t(alist);
	fd = va_arg_int(alist);
	/*
	 * here constness isn't respected for write operations, but it
	 * simplifies greatly the code
	 */
	buf = va_arg_ptr(alist, void *);
	size = va_arg_size_t(alist);

	sret = do_tar_read_write(tar, fd, buf, size);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
	va_return_ssize_t(alist, sret);
#pragma GCC diagnostic pop
}

static void tar_in_serialize(struct tar *tar)
{
	int ret;

	ret = tar_append_tree(tar->tar, tar->path, basename(tar->path));
	if (ret < 0) {
		tar->err = errno;
		return;
	}
	ret = tar_append_eof(tar->tar);
	if (ret < 0) {
		tar->err = errno;
		return;
	}
	ret = tar_close(tar->tar);
	if (ret < 0) {
		tar->err = errno;
		return;
	}

	tar->err = 0;
	tar->eof = true;

	return;
}

static void tar_out_deserialize(struct tar *tar)
{
	int ret;

	ret = tar_extract_all(tar->tar, tar->path);
	if (ret < 0) {
		tar->err = errno;
		return;
	}

	tar->err = 0;
	tar->eof = true;

	return;
}

static void tar_process(void *arg)
{
	struct tar *tar = arg;

	if (is_tar_in(tar))
		tar_in_serialize(arg);
	else
		tar_out_deserialize(arg);

	coro_transfer(&tar->coro, &tar->parent);
}

int tar_init(struct tar *tar, const char *path, enum tar_direction direction)
{
	int ret;
	int flag;

	flag = direction == TAR_READ ? O_WRONLY : O_RDONLY;

	memset(tar, 0, sizeof(*tar));

	tar->ops = tar_ops;
	tar->path = strdup(path);
	if (tar->path == NULL)
		return -errno;

	if (flag == O_RDONLY) {
		tar->ops.readfunc = (readfunc_t)alloc_callback(&tar_callback,
				tar);
		if (tar->ops.readfunc == NULL)
			return -ENOMEM;
	} else {
		tar->ops.writefunc = (writefunc_t)alloc_callback(&tar_callback,
				tar);
		if (tar->ops.writefunc == NULL)
			return -ENOMEM;
	}
	ret = tar_open(&tar->tar, tar->path, &tar->ops, flag, DEFAULT_MODE,
			TAR_GNU);
	if (ret < 0) {
		ch_string_cleanup(&tar->path);
		return -errno;
	}
	coro_create(&tar->parent, NULL, NULL, NULL, 0);
	ret = coro_stack_alloc(&tar->stack, 0);
	if (ret != 1)
		return -ENOMEM;
	coro_create(&tar->coro, tar_process, tar, tar->stack.sptr,
			tar->stack.ssze);

	return 0;
}

bool tar_is_direction_read(const struct tar *tar)
{
	return is_tar_in(tar);
}

static ssize_t tar_read_write(struct tar *tar, void *buf, size_t size)
{
	tar->buf = buf;
	tar->size = size;
	tar->cur = 0;

	if (tar->eof)
		return 0;

	do {
		coro_transfer(&tar->parent, &tar->coro);
	} while (!tar->eof && tar->err == 0 && tar->cur != tar->size);

	return tar->cur;
}

ssize_t tar_write(struct tar *tar, const void *buf, size_t size)
{
	return tar_read_write(tar, (void *)buf, size);
}

ssize_t tar_read(struct tar *tar, void *buf, size_t size)
{
	return tar_read_write(tar, buf, size);
}

void tar_cleanup(struct tar *tar)
{
	coro_stack_free(&tar->stack);
	coro_destroy(&tar->coro);
	coro_destroy(&tar->parent);
	ch_string_cleanup(&tar->path);
}

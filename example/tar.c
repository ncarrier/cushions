#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <error.h>

#include <libtar.h>

static int tar_in_open(const char *path, int flags, ...)
{
	int ret;

	ret = open(path, flags);

	printf("%s(%s, %d) -> %d\n", __func__, path, flags, ret);

	return ret;
}

static int tar_in_close(int fd)
{
	int ret;

	ret = close(fd);

	printf("%s(%d) -> %d\n", __func__, fd, ret);

	return ret;
}

static ssize_t tar_in_read(int fd, void *buf, size_t size)
{
	ssize_t sret;

	sret = read(fd, buf, size);

	printf("%s(%d, %p, %zu) -> %zd\n", __func__, fd, buf, size, sret);

	return sret;
}

static ssize_t tar_in_write(int fd, const void *buf, size_t size)
{
	ssize_t sret;

	sret = write(fd, buf, size);

	printf("%s(%d, %p, %zu) -> %zd\n", __func__, fd, buf, size, sret);

	return sret;
}



tartype_t tar_in_ops = {
	.openfunc = tar_in_open,
	.closefunc = tar_in_close,
	.readfunc = tar_in_read,
	.writefunc = tar_in_write,
};

int main(int argc, char *argv[])
{
	int ret;
	TAR *t;
	const char *path;
	char *directory;

	if (argc < 3)
		error(EXIT_FAILURE, 0, "usage: tar archive_name directory\n");
	path = argv[1];
	directory = argv[2];

	printf("%d\n", O_WRONLY);
	ret = tar_open(&t, path, &tar_in_ops, O_WRONLY | O_CREAT, 0644, TAR_GNU);
	if (ret < 0)
		error(EXIT_FAILURE, errno, "tar_open");
	ret = tar_append_tree(t, directory, basename(directory));
	if (ret < 0)
		error(EXIT_FAILURE, errno, "tar_append_tree");
	ret = tar_append_eof(t);
	if (ret < 0)
		error(EXIT_FAILURE, errno, "tar_append_tree");
	ret = tar_close(t);
	if (ret < 0)
		error(EXIT_FAILURE, errno, "tar_close");

	return EXIT_SUCCESS;
}

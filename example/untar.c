#include <sys/param.h>
#include <fcntl.h>

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <error.h>

#include "tar.h"

#define BUF_SIZE 500

static FILE *src;
static struct tar_out to;

static void cleanup(void)
{
	if (src != NULL)
		fclose(src);

	src = NULL;

	tar_out_cleanup(&to);
}

int main(int argc, char *argv[])
{
	int ret;
	const char *src_path;
	const char *dest_path;
	char buf[BUF_SIZE];
	bool eof;
	size_t sread;
	ssize_t swritten;

	if (argc < 2)
		error(EXIT_FAILURE, 0, "usage: untar tar_file [dest]");
	src_path = argv[1];
	dest_path = argc == 3 ? argv[2] : ".";

	ret = tar_out_init(&to, dest_path);
	if (ret < 0)
		error(EXIT_FAILURE, -ret, "tar_out_init");
	src = fopen(src_path, "rbe");
	if (src == NULL)
		error(EXIT_FAILURE, errno, "fopen(%s)", src_path);
	atexit(cleanup);

	eof = false;
	do {
		sread = fread(buf, 1, BUF_SIZE, src);
		if (sread < BUF_SIZE) {
			ret = errno;
			eof = feof(src);
			if (!eof)
				error(EXIT_FAILURE, ret, "fread");
		}
		swritten = tar_out_write(&to, buf, sread);
		if (swritten != sread) {
			if (swritten < 0)
				error(EXIT_FAILURE, -swritten, "tar_out_write");
			else
				error(EXIT_FAILURE, 0, "EOF on dest");
		}
	} while (!eof);

	return EXIT_SUCCESS;
}

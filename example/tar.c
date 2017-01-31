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

#include "tar.h"

#define BUFFER_SIZE 500

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

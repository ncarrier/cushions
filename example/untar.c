#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>

#include <error.h>

#include "tar.h"

#define BUF_SIZE 100

static void file_cleanup(FILE **file)
{
	if (file == NULL || *file == NULL)
		return;

	fclose(*file);
	*file = NULL;
}

int main(int argc, char *argv[])
{
	int ret;
	size_t size;
	struct tar_out __attribute__((cleanup(tar_out_cleanup))) to = {
			.file = NULL };
	const char *path;
	FILE __attribute__((cleanup(file_cleanup)))*f = NULL;
	bool eof;
	char buf[BUF_SIZE];
	char *p;
	unsigned consumed;
	const char *dest;

	if (argc < 2)
		error(EXIT_FAILURE, 0, "usage: untar tar_file [dest]\n");
	path = argv[1];
	dest = argc == 3 ? argv[2] : ".";


	ret = tar_out_init(&to, dest);
	if (ret < 0)
		error(EXIT_FAILURE, -ret, "tar_out_init");
	f = fopen(path, "rbe");
	if (f == NULL)
		error(EXIT_FAILURE, 0, "fopen %s: %s", path, strerror(errno));

	eof = false;
	do {
		ret = 0;
		size = fread(buf, 1, BUF_SIZE, f);
		if (size < BUF_SIZE) {
			ret = errno;
			eof = feof(f);
			if (!eof)
				error(EXIT_FAILURE, ret, "fread");
		}
		p = buf;
		do {
			consumed = to.o.store_data(&to, p, size);
			if (to.o.is_full(&to)) {
				ret = to.o.process_block(&to);
				if (ret < 0)
					error(EXIT_FAILURE, -ret, "process");
				if (ret == TAR_OUT_END)
					break;
			}
			p += consumed;
			size -= consumed;
		} while (size > 0);
		if (eof)
			if (!to.o.is_empty(&to))
				error(EXIT_FAILURE, 0, "truncated archive");
	} while (!eof && ret != TAR_OUT_END);

	if (ret != TAR_OUT_END)
		error(EXIT_FAILURE, 0, "truncated archive");

	tar_out_cleanup(&to);

	return EXIT_SUCCESS;
}

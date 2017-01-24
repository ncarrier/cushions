#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <error.h>

#define USAGE "usage: cp source destination"
#define BUFFER_SIZE 0x100

static void cleanup_file(FILE **f)
{
	if (f == NULL || *f == NULL)
		return;

	fclose(*f);
	*f = NULL;
}

static int old_main(int argc, char *argv[])
{
	int ret;
	const char *src_path;
	const char *dest_path;
	FILE __attribute__((cleanup(cleanup_file)))*src = NULL;
	FILE __attribute__((cleanup(cleanup_file)))*dest = NULL;
	size_t sread;
	size_t swritten;
	char buf[BUFFER_SIZE];
	bool eof = false;

	if (argc == 1) {
		puts(USAGE);
		return EXIT_SUCCESS;
	}
	if (argc == 2)
		error(EXIT_FAILURE, 0, "missing destination file\n" USAGE);

	src_path = argv[1];
	dest_path = argv[2];

	src = fopen(src_path, "rb");
	ret = errno;
	if (src == NULL)
		error(EXIT_FAILURE, 0, "fopen %s: %s", src_path, strerror(ret));
	dest = fopen(dest_path, "wb");
	ret = errno;
	if (dest == NULL)
		error(EXIT_FAILURE, errno, "fopen %s: %s", dest_path,
				strerror(ret));

	do {
		sread = fread(buf, 1, BUFFER_SIZE, src);
		if (sread != BUFFER_SIZE) {
			if (ferror(src))
				error(EXIT_FAILURE, errno, "fread");
			else
				eof = true;
		}
		swritten = fwrite(buf, 1, sread, dest);
		if (swritten != sread) {
			if (ferror(dest))
				error(EXIT_FAILURE, errno, "fwrite");
			else
				error(EXIT_FAILURE, errno, "EOF on dest");
		}
	} while (!eof);

	fflush(dest);
	fflush(src);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int count = 1;

	while (--count)
		old_main(argc, argv);

	return old_main(argc, argv);
}

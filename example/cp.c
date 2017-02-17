#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <error.h>

#define USAGE "usage: cp source destination"
#define BUFFER_SIZE 0x2000

static void cleanup_file(FILE **f)
{
	if (f == NULL || *f == NULL)
		return;

	fclose(*f);
	*f = NULL;
}

static FILE *src;
static FILE *dest;

static void cleanup(void)
{
	cleanup_file(&dest);
	cleanup_file(&src);
}

static int old_main(int argc, char *argv[])
{
	const char *src_path;
	const char *dest_path;
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
	if (src == NULL)
		error(EXIT_FAILURE, errno, "fopen %s", src_path);
	atexit(cleanup);
	dest = fopen(dest_path, "wb");
	if (dest == NULL)
		error(EXIT_FAILURE, errno, "fopen %s", dest_path);

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
				error(EXIT_FAILURE, 0, "EOF on dest");
		}
	} while (!eof);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int count = 1;

	while (--count)
		old_main(argc, argv);

	return old_main(argc, argv);
}

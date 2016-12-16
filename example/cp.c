#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#include <error.h>

#include <cushion.h>

#define USAGE "usage: cp source destination\n"
#define BUFFER_SIZE 0x100

static void cleanup_file(FILE **f)
{
	if (f == NULL || *f == NULL)
		return;

	fclose(*f);
	*f = NULL;
}

int main(int argc, char *argv[])
{
	const char *src_path;
	const char *dest_path;
	FILE __attribute__((cleanup(cleanup_file)))*src = NULL;
	FILE __attribute__((cleanup(cleanup_file)))*dest = NULL;
	size_t sread;
	size_t swritten;
	char buf[BUFFER_SIZE];
	bool eof = false;

	if (argc == 1)
		error(EXIT_SUCCESS, 0, USAGE);
	if (argc == 2)
		error(EXIT_FAILURE, 0, "missing destination file\n" USAGE);

	src_path = argv[1];
	dest_path = argv[2];

	src = cushion_fopen(src_path, "rb");
	if (src == NULL)
		error(EXIT_FAILURE, errno, "fopen");
	dest = cushion_fopen(dest_path, "wb");
	if (dest == NULL)
		error(EXIT_FAILURE, errno, "fopen");

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
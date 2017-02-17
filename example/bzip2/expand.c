#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <string.h>
#include <errno.h>

#include <bzlib.h>

#define BUFFER_SIZE 0x400

static int usage(int status)
{
	fprintf(stderr, "expand file.bz2\n");

	return status;
}

static void strfree(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

int main(int argc, char *argv[])
{
	FILE *src_file;
	FILE *dst_file;
	BZFILE *b;
	int nBuf;
	char buf[BUFFER_SIZE];
	int bzerror;
	int nWritten;
	char *src;
	char __attribute__((cleanup(strfree)))*dst = NULL;
	size_t len;

	if (argc != 2)
		return usage(EXIT_FAILURE);
	src = argv[1];
	len = strlen(src);
	if (strlen(src) < 5 || strncmp(src + len - 4, ".bz2", 4) != 0)
		return usage(EXIT_FAILURE);
	dst = strdup(src);
	if (dst == NULL)
		error(EXIT_FAILURE, errno, "fopen");
	*(dst + len - 4) = '\0'; /* remove the .bz2 extension */

	src_file = fopen(src, "r");
	if (src_file == NULL)
		error(EXIT_FAILURE, errno, "fopen src");
	dst_file = fopen(dst, "w");
	if (dst_file == NULL)
		error(EXIT_FAILURE, errno, "fopen dst");
	b = BZ2_bzReadOpen(&bzerror, src_file, 0, 0, NULL, 0);
	if (bzerror != BZ_OK) {
		BZ2_bzReadClose(&bzerror, b);
		error(EXIT_FAILURE, 0, "bzReadOpen failed");
	}

	bzerror = BZ_OK;
	while (bzerror == BZ_OK) {
		nBuf = BZ2_bzRead(&bzerror, b, buf, BUFFER_SIZE);
		if (bzerror != BZ_OK)
			break;

		/* do something with buf[0 .. nBuf-1] */
		nWritten = fwrite(buf, 1, nBuf, dst_file);
		if (nWritten != nBuf) {
			if (feof(dst_file))
				error(EXIT_FAILURE, 0,
						"unexpected EOF on dest\n");
			else
				error(EXIT_FAILURE, errno, "fwrite");
		}
	}
	if (bzerror != BZ_STREAM_END) {
		BZ2_bzReadClose(&bzerror, b);
		error(EXIT_FAILURE, 0, "bzip2 stream read error\n");
	}

	BZ2_bzReadClose(&bzerror, b);

	return EXIT_SUCCESS;
}

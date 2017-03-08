#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>

#include <libtar.h>

int main(int argc, char *argv[])
{
	TAR *tar;
	int ret;
	char *src_path;
	const char *dst_path;

	if (argc < 3)
		error(EXIT_FAILURE, 0, "usage: tartar archive dest_dir");
	src_path = argv[1];
	dst_path = argv[2];

	ret = tar_open(&tar, src_path, NULL, O_RDONLY, 0644,
			TAR_GNU | TAR_VERBOSE | TAR_IGNORE_CRC |
			TAR_IGNORE_MAGIC);
	if (ret < 0)
		error(EXIT_FAILURE, errno, "tar_open");

	ret = tar_extract_all(tar, (char *)dst_path);
	if (ret < 0)
		error(EXIT_FAILURE, errno, "tar_extract_all");

	ret = tar_close(tar);
	if (ret < 0)
		error(EXIT_FAILURE, errno, "tar_close");

	return EXIT_SUCCESS;
}

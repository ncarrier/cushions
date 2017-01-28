#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "tar.h"

#define HDR_FMT "%100s%Lo\0"

static void file_cleanup(FILE **file)
{
	if (file == NULL || *file == NULL)
		return;

	fclose(*file);
	*file = NULL;
}

static enum type_flag char_to_type_flag(char flag)
{
	switch (flag) {
	case TYPE_FLAG_REGULAR_OBSOLETE:
	case TYPE_FLAG_REGULAR:
	case TYPE_FLAG_LINK:
	case TYPE_FLAG_SYMLINK:
	case TYPE_FLAG_CHAR:
	case TYPE_FLAG_BLOCK:
	case TYPE_FLAG_DIRECTORY:
	case TYPE_FLAG_FIFO:
	case TYPE_FLAG_CONTIGUOUS:
		return (enum type_flag)flag;

	default:
		return 'z';
	}
}

static int tar_out_convert_header(struct tar_out *to)
{
	struct header *h;
	struct block_header_raw *hr;
	char *endptr;

	h = &to->header;
	hr = &to->raw_header;

	/* TODO fields may be unused */

	memset(h, 0, sizeof(*h));
	if (*(hr->prefix) != '\0')
		snprintf(h->path, PATH_LEN, "%.*s/%.*s", PREFIX_LEN, hr->prefix,
				NAME_LEN, hr->name);
	else
		memcpy(h->path, hr->name, sizeof(hr->name));
	h->mode = strtoul(hr->mode, &endptr, 8);
	if (*hr->mode == '\0' || *endptr != '\0')
		return -EINVAL;
	h->uid = strtoul(hr->uid, &endptr, 8);
	if (*hr->uid == '\0' || *endptr != '\0')
		return -EINVAL;
	h->gid = strtoul(hr->gid, &endptr, 8);
	if (*hr->gid == '\0' || *endptr != '\0')
		return -EINVAL;
	h->size = strtoull(hr->size, &endptr, 8);
	if (*hr->size == '\0' || *endptr != '\0')
		return -EINVAL;
	h->mtime = strtoull(hr->mtime, &endptr, 8);
	if (*hr->mtime == '\0' || *endptr != '\0')
		return -EINVAL;
	h->checksum = strtoul(hr->checksum, &endptr, 8);
	if (*hr->checksum == '\0' || *endptr != '\0')
		return -EINVAL;
	h->type_flag = char_to_type_flag(hr->type_flag);
	memcpy(h->link_name, hr->link_name, sizeof(hr->link_name));
	memcpy(h->version, hr->version, sizeof(hr->version));
	snprintf(h->uname, sizeof(h->uname), "%s", hr->uname);
	snprintf(h->gname, sizeof(h->gname), "%s", hr->gname);
	h->devmajor = strtoul(hr->devmajor, &endptr, 8);
	if (*endptr != '\0')
		return -EINVAL;
	h->devminor = strtoul(hr->devminor, &endptr, 8);
	if (*endptr != '\0')
		return -EINVAL;

	return 0;
}

static int tar_out_store_data(struct tar_out *to, const char *buf, size_t size)
{
	size_t room;
	size_t consumed;

	room = BLOCK_SIZE - to->cur;
	consumed = MIN(room, size);
	memcpy(to->data + to->cur, buf, consumed);
	to->cur += consumed;

	return consumed;
}

static bool tar_out_is_write_ongoing(const struct tar_out *to)
{
	return to->file != NULL;
}

__attribute__((unused))
static const char *type_flag_to_str(enum type_flag type_flag)
{
	switch (type_flag) {
	case TYPE_FLAG_REGULAR_OBSOLETE:
		return "regular_obsolete";
	case TYPE_FLAG_REGULAR:
		return "regular";
	case TYPE_FLAG_LINK:
		return "link";
	case TYPE_FLAG_SYMLINK:
		return "symlink";
	case TYPE_FLAG_CHAR:
		return "char";
	case TYPE_FLAG_BLOCK:
		return "block";
	case TYPE_FLAG_DIRECTORY:
		return "directory";
	case TYPE_FLAG_FIFO:
		return "fifo";
	case TYPE_FLAG_CONTIGUOUS:
		return "contiguous";

	default:
		return "(invalid type flag)";
	}
}

static int tar_out_create_regular_file(struct tar_out *to)
{
	const struct header *h;
	int fd;

	h = &to->header;

	fd = openat(to->dest, h->path, O_CLOEXEC | O_EXCL | O_CREAT | O_WRONLY,
			h->mode);
	to->file = fdopen(fd, "wbex");
	if (to->file == NULL)
		return -errno;
	to->remaining = h->size;

	return 0;
}

static int tar_out_create_directory(const struct tar_out *to)
{
	int ret;
	const struct header *h;

	h = &to->header;

	ret = mkdirat(to->dest, h->path, h->mode);
	if (ret < 0)
		return ret;

	return 0;
}

static int tar_out_create_link(const struct tar_out *to)
{
	int ret;
	const struct header *h;

	h = &to->header;
	ret = linkat(to->dest, h->link_name, to->dest, h->path, 0);
	if (ret < 0)
		return -errno;

	return 0;
}

static int tar_out_create_symlink(const struct tar_out *to)
{
	int ret;
	const struct header *h;

	h = &to->header;
	ret = symlinkat(h->link_name, to->dest, h->path);
	if (ret < 0)
		return -errno;

	return 0;
}

static int tar_out_create_device(const struct tar_out *to)
{
	int ret;
	const struct header *h;
	dev_t dev;
	mode_t mode;

	h = &to->header;
	dev = makedev(h->devmajor, h->devminor);
	mode = (h->type_flag == TYPE_FLAG_CHAR ? S_IFCHR : S_IFBLK) | h->mode;
	ret = mknodat(to->dest, to->header.path, mode, dev);
	if (ret == -1)
		return -errno;

	return 0;
}

static int tar_out_create_fifo(const struct tar_out *to)
{
	int ret;
	const struct header *h;

	h = &to->header;
	ret = mkfifoat(to->dest, to->header.path, h->mode);
	if (ret == -1)
		return -errno;

	return 0;
}

static int tar_out_set_metadata(struct tar_out *to)
{
	// TODO set uid and gid
	// TODO set h->mtime
	return 0;
}

static int tar_out_create_node(struct tar_out *to)
{
	int ret;
	struct header *h;

	h = &to->header;

	switch (h->type_flag) {
	case TYPE_FLAG_REGULAR_OBSOLETE:
	case TYPE_FLAG_REGULAR:
	case TYPE_FLAG_CONTIGUOUS:
		ret = tar_out_create_regular_file(to);
		break;

	case TYPE_FLAG_DIRECTORY:
		ret = tar_out_create_directory(to);
		break;

	case TYPE_FLAG_LINK:
		ret = tar_out_create_link(to);
		break;

	case TYPE_FLAG_SYMLINK:
		ret = tar_out_create_symlink(to);
		break;

	case TYPE_FLAG_CHAR:
	case TYPE_FLAG_BLOCK:
		ret = tar_out_create_device(to);
		break;

	case TYPE_FLAG_FIFO:
		ret = tar_out_create_fifo(to);
		break;

	default:
		fprintf(stderr, "unsupported archive member type '%c'\n",
				to->raw_header.type_flag);
		return 0;
	}
	if (ret < 0)
		return ret;

	return tar_out_set_metadata(to);
}

static bool tar_out_header_is_valid(const struct tar_out *to)
{
	unsigned long cs;
	const uint8_t *p;
	const uint8_t *stop;
	const struct block_header_raw *rh;

	/* test the checksum */
	stop = to->data + 512;
	rh = &to->raw_header;
	for (p = to->data; p < stop; p++) {
		if (p >= (uint8_t *)rh->checksum &&
				p < (uint8_t *)&rh->type_flag)
			cs += ' ';
		else
			cs += *p;
	}

	// TODO check other fields

	return cs == to->header.checksum;
}

static int tar_out_process_header(struct tar_out *to)
{
	int ret;

	if (tar_out_header_is_valid(to))
		return -EINVAL;

	ret = tar_out_convert_header(to);
	if (ret < 0)
		return ret;

	return tar_out_create_node(to);
}

static bool tar_out_file_is_finished(const struct tar_out *to)
{
	return to->remaining == 0;
}

static int tar_out_process_data(struct tar_out *to)
{
	int ret;
	size_t sret;
	size_t to_write;

	to_write = MIN(to->remaining, BLOCK_SIZE);

	sret = fwrite(to->data, to_write, 1, to->file);
	if (sret < 1) {
		ret = errno;
		return ferror(to->file) ? ret : EIO;
	}
	to->remaining -= to_write;

	if (tar_out_file_is_finished(to))
		file_cleanup(&(to->file));

	return 0;
}
static void tar_out_consume_block(struct tar_out *to)
{
	to->cur = 0;
}

static bool tar_out_block_is_zero(const struct tar_out *to)
{
	static const char zero_block[BLOCK_SIZE];

	return memcmp(to->data, zero_block, BLOCK_SIZE) == 0;
}

static int tar_out_process_block(struct tar_out *to)
{
	int ret;

	if (tar_out_block_is_zero(to)) {
		/* end processing at the second zero block found */
		if (to->zero_block_found)
			return TAR_OUT_END;
		to->zero_block_found = true;
	} else {
		to->zero_block_found = false;
	}

	if (tar_out_is_write_ongoing(to))
		ret = tar_out_process_data(to);
	else
		ret = to->zero_block_found ? 0 : tar_out_process_header(to);

	tar_out_consume_block(to);

	return ret;
}

static bool tar_out_is_full(const struct tar_out *to)
{
	return to->cur >= BLOCK_SIZE;
}

static bool tar_out_is_empty(const struct tar_out *to)
{
	return to->cur == 0;
}

static void tar_out_reset(struct tar_out *to)
{
	memset(to, 0, sizeof(*to));

	to->o = (struct tar_out_ops) {
			.is_empty = tar_out_is_empty,
			.is_full = tar_out_is_full,
			.process_block = tar_out_process_block,
			.store_data = tar_out_store_data,
	};
}

int tar_out_init(struct tar_out *to, const char *dest)
{
	tar_out_reset(to);
	to->dest = open(dest, O_DIRECTORY | O_PATH | O_CLOEXEC);
	if (to->dest == -1)
		return -errno;

	return 0;
}

void tar_out_cleanup(struct tar_out *to)
{
	if (to->file != NULL)
		file_cleanup(&to->file);
	tar_out_reset(to);
}

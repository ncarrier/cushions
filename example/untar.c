#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>

#include <error.h>

#define TAR_OUT_END 1

#define BUF_SIZE 100
#define BLOCK_SIZE 512

#define NAME_LEN 100
#define MODE_LEN 8
#define UID_LEN 8
#define GID_LEN 8
#define SIZE_LEN 12
#define MTIME_LEN 12
#define CHECKSUM_LEN 8
#define LINK_LEN 100
#define MAGIC_LEN 6
#define VERSION_LEN 2
#define UNAME_LEN 32
#define GNAME_LEN 32
#define DEVMAJOR_LEN 8
#define DEVMINOR_LEN 8
#define PREFIX_LEN 155

/* prefix + '/' + name + '\0' */
#define PATH_LEN (PREFIX_LEN + 1 + NAME_LEN + 1)

struct block_header_raw {
	char name[NAME_LEN];         /* NUL-terminated if NUL fits */
	char mode[MODE_LEN];
	char uid[UID_LEN];
	char gid[GID_LEN];
	char size[SIZE_LEN];
	char mtime[MTIME_LEN];
	char checksum[CHECKSUM_LEN];
	char type_flag;
	char link_name[LINK_LEN];    /* NUL-terminated if NUL fits */
	char magic[MAGIC_LEN];       /* must be TMAGIC (NUL term.) */
	char version[VERSION_LEN];   /* must be TVERSION */
	char uname[UNAME_LEN];       /* NUL-terminated */
	char gname[GNAME_LEN];       /* NUL-terminated */
	char devmajor[DEVMAJOR_LEN];
	char devminor[DEVMINOR_LEN];
	char prefix[PREFIX_LEN];     /* NUL-terminated if NUL fits */
} __attribute__((packed));

enum type_flag {
	TYPE_FLAG_REGULAR_OBSOLETE = '\0',
	TYPE_FLAG_REGULAR          = '0',
	TYPE_FLAG_LINK             = '1',
	TYPE_FLAG_SYMLINK          = '2',
	TYPE_FLAG_CHAR             = '3',
	TYPE_FLAG_BLOCK            = '4',
	TYPE_FLAG_DIRECTORY        = '5',
	TYPE_FLAG_FIFO             = '6',
	TYPE_FLAG_CONTIGUOUS       = '7',
};

#define HDR_FMT "%100s%Lo\0"

struct header {
	char path[PATH_LEN];
	unsigned long mode;
	unsigned long uid;
	unsigned long gid;
	unsigned long long size;
	unsigned long long mtime;
	unsigned long checksum;
	enum type_flag type_flag;
	char link_name[LINK_LEN + 1];
	char version[VERSION_LEN];
	char uname[UNAME_LEN];
	char gname[GNAME_LEN];
	unsigned long devmajor;
	unsigned long devminor;
};

struct block {
	union {
		struct block_header_raw header;
		uint8_t data[BLOCK_SIZE];
	};
} __attribute__((packed));

struct tar_out {
	struct block block;
	struct header header;
	unsigned cur;
	FILE *file;
	unsigned remaining;
	bool zero_block_found;
};

static void file_cleanup(FILE **file)
{
	if (file == NULL || *file == NULL)
		return;

	fclose(*file);
	*file = NULL;
}

static int tar_out_convert_header(struct tar_out *to)
{
	struct header *h;
	struct block_header_raw *hr;
	char *endptr;

	h = &to->header;
	hr = &to->block.header;

	/* TODO fields may be unused */

	memset(h, 0, sizeof(*h));
	if (*(hr->prefix) != '\0')
		snprintf(h->path, PATH_LEN, "%.*s/%.*s", PREFIX_LEN, hr->prefix,
				NAME_LEN, hr->name);
	else
		memcpy(h->path, hr->name, sizeof(hr->name));
	printf("path is %s\n", h->path);
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
	// TODO proper conversion function
	h->type_flag = (enum type_flag)hr->type_flag;
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

static void tar_out_init(struct tar_out *to)
{
	memset(to, 0, sizeof(*to));
}

static int tar_out_store_data(struct tar_out *to, const char *buf, size_t size)
{
	size_t room;
	size_t consumed;

	room = BLOCK_SIZE - to->cur;
	consumed = MIN(room, size);
	memcpy(to->block.data + to->cur, buf, consumed);
	to->cur += consumed;

	return consumed;
}

static bool tar_out_is_write_ongoing(const struct tar_out *to)
{
	return to->file != NULL;
}
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
	int ret;
	struct header *h;
	int fd;

	h = &to->header;

//	TODO use openat and fdopen
	to->file = fopen(h->path, "wbex");
	if (to->file == NULL)
		return -errno;
	fd = fileno(to->file);
	if (fd == -1)
		return -errno;
	ret = fchmod(fd, h->mode);
	if (ret == -1)
		return -errno;
	to->remaining = h->size;

	return 0;
}

static int tar_out_create_directory(struct tar_out *to)
{
	int ret;
	struct header *h;

	h = &to->header;

	ret = mkdirat(AT_FDCWD, h->path, h->mode);
	if (ret < 0)
		return ret;

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
	case TYPE_FLAG_SYMLINK:
	case TYPE_FLAG_CHAR:
	case TYPE_FLAG_BLOCK:
	case TYPE_FLAG_FIFO:
		fprintf(stderr, "type flag '%s' not yet supported",
				type_flag_to_str(h->type_flag));
		return 0;

	default:
		return -EINVAL;
	}
	if (ret < 0)
		return ret;

	return tar_out_set_metadata(to);
}

static int tar_out_process_header(struct tar_out *to)
{
	int ret;

	// TODO verify header

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

	sret = fwrite(to->block.data, to_write, 1, to->file);
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

static bool tar_block_is_zero(const struct tar_out *to)
{
	static const char zero_block[BLOCK_SIZE];

	return memcmp(to->block.data, zero_block, BLOCK_SIZE) == 0;
}

static int tar_out_process_block(struct tar_out *to)
{
	int ret;

	if (tar_block_is_zero(to)) {
		/* end processing at the second zero block found */
		if (to->zero_block_found)
			return TAR_OUT_END;
		to->zero_block_found = true;
	} else {
		to->zero_block_found = false;
	}

	if (tar_out_is_write_ongoing(to)) {
		ret = tar_out_process_data(to);
	} else {
		if (!to->zero_block_found)
			ret = tar_out_process_header(to);
	}

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

static void tar_out_cleanup(struct tar_out *to)
{
	if (to->file != NULL)
		file_cleanup(&to->file);
	tar_out_init(to);
}

int main(int argc, char *argv[])
{
	int ret;
	size_t size;
	struct tar_out to;
	const char *path;
	FILE __attribute__((cleanup(file_cleanup)))*f = NULL;
	bool eof;
	char buf[BUF_SIZE];
	char *p;
	unsigned consumed;

	if (argc < 2)
		error(EXIT_FAILURE, 0, "usage: untar tar_file\n");
	path = argv[1];

	tar_out_init(&to);
	f = fopen(path, "rbe");
	if (f == NULL)
		error(EXIT_FAILURE, 0, "fopen %s: %s", path, strerror(errno));

	eof = false;
	do {
		size = fread(buf, 1, BUF_SIZE, f);
		if (size < BUF_SIZE) {
			ret = errno;
			eof = feof(f);
			if (!eof)
				error(EXIT_FAILURE, ret, "fread");
		}
		p = buf;
		do {
			consumed = tar_out_store_data(&to, p, size);
			if (tar_out_is_full(&to)) {
				ret = tar_out_process_block(&to);
				if (ret < 0)
					error(EXIT_FAILURE, -ret, "process");
				if (ret == TAR_OUT_END)
					break;
			}
			p += consumed;
			size -= consumed;
		} while (size > 0);
		if (eof)
			if (!tar_out_is_empty(&to))
				error(EXIT_FAILURE, 0, "truncated archive");
	} while (!eof && ret != TAR_OUT_END);

	tar_out_cleanup(&to);

	return EXIT_SUCCESS;
}

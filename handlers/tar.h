#ifndef HANDLERS_TAR_H_
#define HANDLERS_TAR_H_
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#define TAR_OUT_END 1

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

	TYPE_FLAG_INVALID          = 'z',
};

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

struct tar_out;

struct tar_out_ops {
	bool (*is_empty)(const struct tar_out *to);
	bool (*is_full)(const struct tar_out *to);
	int (*process_block)(struct tar_out *to);
	int (*store_data)(struct tar_out *to, const char *buf, size_t size);
};

struct tar_out_directory {
	unsigned long long mtime;
	char path[PATH_LEN];
};

struct tar_out {
	union {
		struct block_header_raw raw_header;
		uint8_t data[BLOCK_SIZE];
	};
	int dest;
	unsigned cur;
	FILE *file;
	unsigned remaining;
	struct tar_out_ops o;
	struct header header;
	bool zero_block_found;
	struct tar_out_directory *directories;
	unsigned nb_directories;
	unsigned cur_directory;
};

int tar_out_init(struct tar_out *to, const char *dest);

void tar_out_cleanup(struct tar_out *to);

#endif /* HANDLERS_TAR_H_ */

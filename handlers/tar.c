#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "tar.h"

#define HDR_FMT "%100s%Lo\0"
#define INITIAL_DIR_NB 8
#define DEFAULT_UMASK 0022

#define RESUME_CONTINUE 1
#define RESUME_FULLFILLED 2
#define RESUME_ENDED 3

struct tar_in_coroutine_arg {
	void *buf;
	size_t size;
	size_t cur;
	struct tar_in *ti;
};

static void string_cleanup(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

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
	// TODO sanitize paths (just remove leading '/'s ?)
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

static int tar_out_store_directory(struct tar_out *to)
{
	struct tar_out_directory *temp;
	struct tar_out_directory *dir;

	/* grow buffer */
	if (to->cur_directory == to->nb_directories) {
		if (to->nb_directories == 0)
			to->nb_directories = INITIAL_DIR_NB;
		else
			to->nb_directories <<= 1;
		temp = realloc(to->directories,
				to->nb_directories * sizeof(*to->directories));
		if (temp == NULL)
			return -errno;
		to->directories = temp;
	}

	dir = to->directories + to->cur_directory;
	dir->mtime = to->header.mtime;
	snprintf(dir->path, PATH_LEN, "%s", to->header.path);
	to->cur_directory++;

	return 0;
}

static int tar_out_create_directory(struct tar_out *to, int dest,
		const char *path, unsigned long mode)
{
	int ret;

	ret = mkdirat(dest, path, mode);
	if (ret < 0)
		return -errno;

	return tar_out_store_directory(to);
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

static int set_mtime(int destfd, const char *path, unsigned long long mtime)
{
	int ret;
	const struct timespec times[2] = {
			[0] = {
					.tv_sec = 0,
					.tv_nsec = UTIME_OMIT,
			},
			[1] = {
					.tv_sec = mtime,
					.tv_nsec = 0,
			},
	};

	ret = utimensat(destfd, path, times, AT_SYMLINK_NOFOLLOW);
	if (ret < 0)
		perror("utimensat");

	return 0;
}

static int tar_out_set_ids(const struct tar_out *to)
{
	int ret;
	uid_t uid;
	gid_t gid;
	struct passwd *pwd;
	struct group *grp;

	pwd = getpwnam(to->header.uname);
	grp = getgrnam(to->header.gname);
	uid = pwd == NULL ? to->header.uid : pwd->pw_uid;
	gid = pwd == NULL ? to->header.gid : grp->gr_gid;

	ret = fchownat(to->dest, to->header.path, uid, gid,
			AT_SYMLINK_NOFOLLOW);
	if (ret < 0)
		perror("fchownat");

	return 0;
}

static mode_t process_umask(void)
{
	int ret;
	const char *status_path = "/proc/self/status";
	FILE __attribute__((cleanup(file_cleanup)))*status_file = NULL;
	unsigned umask;
	char __attribute__((cleanup(string_cleanup)))*line = NULL;
	size_t line_len = 0;

	status_file = fopen(status_path, "rb");
	if (NULL == status_file)
		return DEFAULT_UMASK;

	while (getline(&line, &line_len, status_file) != -1) {
		if (strncmp("Umask:", line, 6) != 0)
			continue;
		ret = sscanf(line, "Umask:\t%o", &umask);
		if (ret == 1)
			return umask;
	}

	return DEFAULT_UMASK;
}

static int create_missing_path_components(struct tar_out *to)
{
	int ret;
	char __attribute__((cleanup(string_cleanup)))*path = NULL;
	char __attribute__((cleanup(string_cleanup)))*component = NULL;
	char *sep;
	size_t len;
	int component_len;

	path = strdup(to->header.path);
	if (path == NULL)
		return -errno;
	len = strlen(path);
	component = calloc(len + 1, sizeof(*component));

	for (sep = strchr(path, '/'); sep != NULL; sep = strchr(sep + 1, '/')) {
		component_len = sep - path;
		/* path starts with '/', this is highly suspicious... */
		if (component_len == 0)
			continue;
		/*
		 * stop at the full node path, we can create it now
		 * should not happen in fact, the last '/' should be before the
		 * element we wanted to create in the first place, thanks to
		 * canoricalize_file_name
		 */
		if ((unsigned)component_len >= len - (path[len - 1] == '/'))
			break;
		snprintf(component, len + 1, "%.*s", component_len, path);
		ret = faccessat(to->dest, component, F_OK, AT_SYMLINK_NOFOLLOW);
		if (ret == 0)
			continue;

		ret = tar_out_create_directory(to, to->dest, component,
				to->default_dir_mode);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int tar_out_create_node(struct tar_out *to)
{
	int ret;
	struct header *h;
	bool do_set_mtime;

	h = &to->header;
	ret = create_missing_path_components(to);
	if (ret < 0)
		return ret;

	do_set_mtime = true;
	switch (h->type_flag) {
	case TYPE_FLAG_REGULAR_OBSOLETE:
	case TYPE_FLAG_REGULAR:
	case TYPE_FLAG_CONTIGUOUS:
		/* metadata setting must be done after the last modification */
		do_set_mtime = false;
		ret = tar_out_create_regular_file(to);
		break;

	case TYPE_FLAG_DIRECTORY:
		do_set_mtime = false;
		ret = tar_out_create_directory(to, to->dest, h->path, h->mode);
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

	if (to->set_ids) {
		ret = tar_out_set_ids(to);
		if (ret < 0)
			return ret;
	}

	return do_set_mtime ? set_mtime(to->dest, h->path, h->mtime) : 0;
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
	cs = 0;
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

	ret = tar_out_convert_header(to);
	if (ret < 0)
		return ret;

	if (!tar_out_header_is_valid(to))
		return -EINVAL;

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

	if (tar_out_file_is_finished(to)) {
		file_cleanup(&(to->file));
		ret = set_mtime(to->dest, to->header.path, to->header.mtime);
		if (ret < 0)
			return ret;
	}

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

static int tar_out_fix_directories_mtime(struct tar_out *to)
{
	int ret;
	struct tar_out_directory *dir;

	while (to->cur_directory-- != 0) {
		dir = to->directories + to->cur_directory;
		ret = set_mtime(to->dest, dir->path, dir->mtime);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int tar_out_process_block(struct tar_out *to)
{
	int ret;

	if (tar_out_block_is_zero(to)) {
		/* end processing at the second zero block found */
		if (to->zero_block_found) {
			ret = tar_out_fix_directories_mtime(to);
			if (ret < 0)
				return ret;

			return TAR_OUT_END;
		}
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

	to->set_ids = getuid() == 0;

	/* according to
	 * http://pubs.opengroup.org/onlinepubs/009696799/utilities/mkdir.html
	 * this is the mode a missing component directory will have when
	 * mkdir -p is used, we replicate this here
	 */
	to->default_dir_mode = (0777 & ~process_umask()) | S_IWUSR | S_IXUSR;

	return 0;
}

void tar_out_cleanup(struct tar_out *to)
{
	if (to->dest > 0)
		close(to->dest);
	if (to->file != NULL)
		file_cleanup(&to->file);
	if (to->directories != NULL)
		free(to->directories);
	tar_out_reset(to);
}

static int func_tar_in_open(const char *path, int flags, ...)
{
	/*
	 * "opening" operation have already occurred before and the return
	 * value is not significant
	 */
	printf("%s(%s, %d) -> 0\n", __func__, path, flags);

	return 0;
}

static int func_tar_in_close(int fd)
{
	ssize_t sret = RESUME_ENDED;

	yield(&sret);

	printf("%s(%d) -> 0\n", __func__, fd);

	return 0;
}

static ssize_t func_tar_in_read(int fd, void *buf, size_t size)
{
	assert(false);
	return 0;
}

static ssize_t func_tar_in_write(int fd, const void *buf, size_t size)
{
	ssize_t sret;
	struct tar_in_coroutine_arg *arg;
	unsigned consumed;
	unsigned needed;
	size_t remaining;

	remaining = size;
	do {
		sret = RESUME_CONTINUE;
		arg = yield(&sret);
		needed = arg->size - arg->cur;
		consumed = MIN(needed, remaining);
		memcpy((char *)arg->buf + arg->cur, buf, consumed);
		arg->cur += consumed;
		remaining -= consumed;
		buf = ((char *)buf + consumed);
		if (arg->cur == arg->size) {
			sret = RESUME_FULLFILLED;
			arg = yield(&sret);
		}
	} while (remaining != 0);

	printf("%s(%d, %p, %zu) -> %zd\n", __func__, fd, buf, size, sret);

	return size;
}

tartype_t tar_in_ops = {
	.openfunc = func_tar_in_open,
	.closefunc = func_tar_in_close,
	.readfunc = func_tar_in_read,
	.writefunc = func_tar_in_write,
};

static void *tar_in_serialize(void *arg)
{
	struct tar_in_coroutine_arg *a = arg;
	struct tar_in *ti = a->ti;
	int ret;

	ret = tar_append_tree(ti->tar, ti->src, basename(ti->src));
	if (ret < 0)
		return NULL;
	ret = tar_append_eof(ti->tar);
	if (ret < 0)
		return NULL;
	ret = tar_close(ti->tar);
	if (ret < 0)
		return NULL;

	errno = 0;

	return NULL;
}

int tar_in_init(struct tar_in *ti, const char *src)
{
	int ret;

	memset(ti, 0, sizeof(*ti));

	ti->src = strdup(src);
	if (ti->src == NULL)
		return -errno;
	ret = tar_open(&ti->tar, src, &tar_in_ops, O_WRONLY | O_CREAT,
			TAR_IN_DEFAULT_MODE,
			TAR_GNU);
	if (ret < 0) {
		string_cleanup(&ti->src);
		return -errno;
	}
	ti->c = coroutine(tar_in_serialize);

	return 0;
}

ssize_t tar_in_read(struct tar_in *ti, void *buf, size_t size)
{
	struct tar_in_coroutine_arg arg = {
			.buf = buf,
			.size = size,
			.ti = ti,
			.cur = 0,
	};
	ssize_t *sret;

	if (!resumable(ti->c)) {
		errno = 0;
		return 0;
	}
	do {
		sret = resume(ti->c, &arg);
		if (sret == NULL)
			return errno == 0 ? 0 : -1;
	} while (*sret == RESUME_CONTINUE);

	/* make func_tar_in_close return */
	if (*sret == RESUME_ENDED)
		resume(ti->c, NULL);

	printf("%s(%p, %zu) -> %zu\n", __func__, arg.buf, size, arg.cur);

	return arg.cur;
}

void tar_in_cleanup(struct tar_in *ti)
{
	string_cleanup(&ti->src);
}

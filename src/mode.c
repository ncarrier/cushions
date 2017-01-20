#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>

#define LOG_TAG mode
#include "cushions_handler.h"

#define CODED_CHARACTER_SET_ANCHOR ",ccs="

int cushions_handler_mode_from_string(struct cushions_handler_mode *mode,
		const char *s)
{
	char *ccs;
	char *str;
	const char *needle;
	size_t invalid;

	if (mode == NULL || s == NULL)
		return -EINVAL;
	str = strdup(s);
	if (str == NULL)
		return -errno;
	memset(mode, 0, sizeof(*mode));
	mode->mode = str;
	ccs = strstr(str, CODED_CHARACTER_SET_ANCHOR);
	if (ccs != NULL) {
		mode->ccs = strdup(ccs +
				sizeof(CODED_CHARACTER_SET_ANCHOR) - 1);
		if (mode->ccs == NULL)
			return -errno;
		*ccs = '\0';
	}
	if (*str == '\0')
		goto err;
	invalid = strspn(str, "abcemrwx+");
	if (invalid != strlen(str)) {
		LOGE("format character %c not allowed in mode %s",
				str[invalid], s);
		goto err;
	}
	if (strchr(str, 'r')) {
		mode->read = 1;
		mode->beginning = 1;
		if (strchr(str, '+'))
			mode->write = 1;
	} else if (strchr(str, 'w')) {
		mode->write = 1;
		mode->truncate = 1;
		mode->beginning = 1;
		if (strchr(str, '+')) {
			mode->read = 1;
			mode->create = 1;
		}
	} else if (strchr(str, 'a')) {
		mode->append = 1;
		mode->end = 1;
		mode->write = 1;
		mode->create = 1;
		if (strchr(str, '+')) {
			mode->beginning = 1;
			mode->read = 1;
		}
	}

	needle = strchr(str, 'b');
	if (needle != NULL)
		mode->binary = 1;
	if (needle == str)
		goto err;
	
	if (strchr(str, 'c'))
		mode->no_cancellation = 1;
	if (strchr(str, 'e'))
		mode->cloexec = 1;
	if (strchr(str, 'x'))
		mode->excl = 1;
	if (strchr(str, 'm'))
		mode->mmap = 1;

	/* restore the original format string with it's , */
	if (ccs != NULL)
		*ccs = CODED_CHARACTER_SET_ANCHOR[0];

	return 0;
err:
	cushions_handler_mode_cleanup(mode);

	return -EINVAL;
}

int cushions_handler_mode_to_string(const struct cushions_handler_mode *mode,
		char **str)
{
	int ret;
	char prefix[8] = {0};
	char *p = prefix;

	if (mode == NULL)
		return -EINVAL;

	if (mode->append) {
		*(p++) = 'a';
		if (mode->beginning)
			*(p++) = '+';
	} else if (mode->truncate) {
		*(p++) = 'w';
		if (mode->read)
			*(p++) = '+';
	} else {
		*(p++) = 'r';
		if (mode->write)
			*(p++) = '+';
	}
	if (mode->binary)
		*(p++) = 'b';
	if (mode->no_cancellation)
		*(p++) = 'c';
	if (mode->cloexec)
		*(p++) = 'e';
	if (mode->mmap)
		*(p++) = 'm';
	if (mode->excl)
		*(p++) = 'x';

	ret = asprintf(str, "%s%s", prefix, mode->ccs != NULL ? mode->ccs : "");
	if (ret < 0)
		return -ENOMEM;

	return 0;
}

void cushions_handler_mode_cleanup(struct cushions_handler_mode *mode)
{
	if (mode == NULL)
		return;

	string_cleanup(&mode->mode);
	string_cleanup(&mode->ccs);
}

void cushions_handler_mode_dump(const struct cushions_handler_mode *mode)
{
	if (mode == NULL)
		return;
	LOGD("mode is [%s%s%s%s%s%s%s%s%s%s%s%sccs=%s]",
			mode->read ? "read " : "",
			mode->beginning ? "beginning " : "",
			mode->end ? "end " : "",
			mode->write ? "write " : "",
			mode->truncate ? "truncate " : "",
			mode->create ? "create " : "",
			mode->binary ? "binary " : "",
			mode->append ? "append " : "",
			mode->no_cancellation ? "no_cancellation " : "",
			mode->cloexec ? "cloexec " : "",
			mode->mmap ? "mmap " : "",
			mode->excl ? "excl " : "",
			mode->ccs != NULL ? mode->ccs : "");
}


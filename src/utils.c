#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/cushions/cushions_handler_utils.h"

bool string_matches_prefix(const char *string, const char *prefix)
{
	return strncmp(string, prefix, strlen(prefix)) == 0;
}

void string_cleanup(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

void fd_cleanup(int *fd)
{
	if (fd == NULL || *fd < 1)
		return;

	close(*fd);
	*fd = -1;
}

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/cushions/cushions_handler_utils.h"

bool ch_string_matches_prefix(const char *string, const char *prefix)
{
	return strncmp(string, prefix, strlen(prefix)) == 0;
}

void ch_string_cleanup(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

void ch_fd_cleanup(int *fd)
{
	if (fd == NULL || *fd < 1)
		return;

	close(*fd);
	*fd = -1;
}

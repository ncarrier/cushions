#include <cushions_common.h>
#include <cushions.h>

#include <stdio.h>

CUSHIONS_API FILE *fopen(const char *path, const char *mode)
{
	return cushions_fopen(path, mode);
}


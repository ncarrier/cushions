#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

/* Function pointers to hold the value of the glibc functions */
static FILE *(*real_fopen)(const char *path, const char *mode);

FILE *fopen(const char *path, const char *mode)
{
	fprintf(stderr, "***** called fopen(%s, %s) *****\n", path, mode);
	real_fopen = dlsym(RTLD_NEXT, "fopen");

	return real_fopen(path, mode);
}


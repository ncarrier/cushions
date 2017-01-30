#include <stdlib.h>
#include <stdio.h>

#include <error.h>

#include "picoro.h"

static void *cr(void *arg)
{
	int *i = arg;
	int j = 0;

	printf("*i = %d\n", *i);
	yield(&j);
	j++;
	printf("*i = %d\n", *i);
	yield(&j);
	j++;

	return NULL;
}

int main(int argc, char *argv[])
{
	coro c;
	int *j;
	int i;

	c = coroutine(cr);
	i = 10;

	while ((j = resume(c, &i)) != NULL) {
		i++;
		printf("*j = %d\n", *j);
	}

	printf("resumable %s\n", resumable(c) ? "true" : "false");

	return EXIT_SUCCESS;
}

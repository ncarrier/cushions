#include <stdlib.h>
#include <stdio.h>

#include "wrap_malloc.h"

void * __real_malloc (size_t c);

void * __wrap_malloc (size_t c)
{
	printf ("malloc called with %zu\n", c);
	return __real_malloc(c);
}

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define VAR_PRINTF(f, ...) printf("[VAR_PRINTF]" f, __VA_ARGS__)

int main(void)
{
	VAR_PRINTF("hello world %d !\n", 5);

	return EXIT_SUCCESS;
}

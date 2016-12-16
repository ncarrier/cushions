#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	char *buf;

	buf = malloc(10);
	snprintf(buf, 10, "tutu");

	printf("hello wrapped %s world !\n", buf);

	return EXIT_SUCCESS;
}

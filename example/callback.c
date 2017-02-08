#include <stdio.h>
#include <stdlib.h>

#include <callback.h>

typedef int (*fun)(int prm);

struct my_type {
	char c;
};

static void function(void *data, va_alist alist)
{
	int prm;
	struct my_type *t = data;

	va_start_int(alist);
	prm = va_arg_int(alist);
	printf("parameter prm is %d, my_type.c is %c\n", prm, t->c);
	va_return_int(alist, -1);
}

int main(int argc, char *argv[])
{
	int ret;
	struct my_type t = { .c = 'c' };
	fun callback;

	callback = alloc_callback(&function, &t);

	ret = callback(12);
	printf("callback returned %d\n", ret);

	free_callback(callback);

	return EXIT_SUCCESS;
}

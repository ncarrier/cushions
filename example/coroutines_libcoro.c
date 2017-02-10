#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <error.h>

#include "coro.h"

struct coroutine {
	struct coro_context coro;
	struct coro_stack stack;
	struct coro_context *parent;
	int i;
};

static int coroutine_init(struct coroutine *c, coro_func func, void *arg,
		struct coro_context *parent)
{
	int ret;

	ret = coro_stack_alloc(&c->stack, 0);
	if (ret != 1)
		return -ENOMEM;
	coro_create(&c->coro, func, arg, c->stack.sptr, c->stack.ssze);
	c->parent = parent;
	c->i = 0;

	return 0;
}

static void coroutine_cleanup(struct coroutine *c)
{

	coro_stack_free(&c->stack);
	coro_destroy(&c->coro);
}

static void cr(void *arg)
{
	struct coroutine *c = arg;

	while (true) {
		printf("*i = %d\n", c->i);
		c->i++;
		coro_transfer(&c->coro, c->parent);
	}
}

int main(int argc, char *argv[])
{
	struct coroutine root;
	struct coroutine c;
	int ret;

	coro_create(&root.coro, NULL, NULL, NULL, 0);
	c.i = 0;
	ret = coroutine_init(&c, cr, &c, &root.coro);
	if (ret < 0)
		error(EXIT_FAILURE, -ret, "coroutine_init");
	do {
		coro_transfer(&root.coro, &c.coro);
		printf("in root, i = %d\n", c.i);
		c.i += 2;
	} while (c.i < 10);

	coroutine_cleanup(&c);

	return EXIT_SUCCESS;
}

/*
 * picoro - minimal coroutines for C.
 * Written by Tony Finch <dot@dotat.at>
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

#include "picoro.h"

#define STACK_SIZE (16 * 1024)

struct coro {
	struct coro *next;
	jmp_buf state;
};

/*
 * Each coroutine has a jmp_buf to hold its context when suspended.
 *
 * There are lists of running and idle coroutines.
 *
 * The coroutine at the head of the running list has the CPU, and all
 * others are suspended inside resume(). The "first" coro object holds
 * the context for the program's initial stack and also ensures that
 * all externally-visible list elements have non-NULL next pointers.
 * (The "first" coroutine isn't exposed to the caller.)
 *
 * The idle list contains coroutines that are suspended in
 * coroutine_main(). After initialization it is never NULL except
 * briefly while coroutine_main() forks a new idle coroutine.
 */
static struct coro first;
static struct coro *running = &first;
static struct coro *idle;

/*
 * A coroutine can be passed to resume() if
 * it is not on the running or idle lists.
 */
bool resumable(coro c)
{
	return c != NULL && c->next == NULL;
}

/*
 * Add a coroutine to a list and return the previous head of the list.
 */
static void push(coro *list, coro c)
{
	c->next = *list;
	*list = c;
}

/*
 * Remove a coroutine from a list and return it.
 */
static coro pop(coro *list)
{
	coro c = *list;
	*list = c->next;
	c->next = NULL;

	return c;
}

/*
 * Pass a value and control from one coroutine to another.
 * The current coroutine's state is saved in "me" and the
 * target coroutine is at the head of the "running" list.
 */
static void *pass(coro me, void *arg)
{
	int ret;
	static void *saved;

	saved = arg;

	ret = setjmp(me->state);
	if (ret == 0)
		longjmp(running->state, 1);

	return saved;
}

__attribute__((noinline))
void *resume(struct coro *c, void *arg)
{
	assert(resumable(c));

	push(&running, c);

	return pass(c->next, arg);
}

__attribute__((noinline))
void *yield(void *arg)
{
	coro c;

	c = pop(&running);

	return pass(c, arg);
}

/* Declare for mutual recursion. */
static void coroutine_start(void);
static void coroutine_main(void *arg);

/*
 * The coroutine constructor function.
 *
 * On the first invocation there are no idle coroutines, so fork the
 * first one, which will immediately yield back to us after becoming
 * idle. When there are idle coroutines, we pass one the function
 * pointer and return the activated coroutine's address.
 */
coro coroutine(void *fun(void *arg))
{
	coro c;

	if (idle == NULL && setjmp(running->state) == 0)
		coroutine_start();
	c = pop(&idle);

	return resume(c, fun);
}

/*
 * The main loop for a coroutine is responsible for managing the "idle" list.
 *
 * When we start the idle list is empty, so we put ourself on it to
 * ensure it remains non-NULL. Then we immediately suspend ourself
 * waiting for the first function we are to run. (The head of the
 * running list is the coroutine that forked us.) We pass the stack
 * pointer to prevent it from being optimised away. The first time we
 * are called we will return to the fork in the coroutine()
 * constructor function (above); on subsequent calls we will resume
 * the parent coroutine_main(). In both cases the passed value is
 * lost when pass() longjmp()s to the forking setjmp().
 *
 * When we are resumed, the idle list is empty again, so we fork
 * another coroutine. When the child coroutine_main() passes control
 * back to us, we drop into our main loop.
 *
 * We are now head of the running list with a function to call. We
 * immediately yield a pointer to our context object so our creator
 * can identify us. The creator can then resume us at which point we
 * pass the argument to the function to start executing.
 *
 * When the function returns, we move ourself from the running list to
 * the idle list, before passing the result back to the resumer. (This
 * is just like yield() except for adding the coroutine to the idle
 * list.) We can then only be resumed by the coroutine() constructor
 * function which will put us back on the running list and pass us a
 * new function to call.
 *
 * We do not declare coroutine_main() static to try to stop it being inlined.
 *
 * The conversion between the function pointer and a void pointer is not
 * allowed by ANSI C but we do it anyway.
 */
__attribute__((noinline))
static void coroutine_main(void *arg)
{
	int ret;
	void *(*fun)(void *);
	struct coro me;
	void *y;
	coro c;

	push(&idle, &me);
	fun = pass(&me, arg);
	ret = setjmp(running->state);
	if (ret == 0)
		coroutine_start();

	while (true) {
		y = yield(&me);
		arg = fun(y);
		c = pop(&running);
		push(&idle, c);
		fun = pass(&me, arg);
	}
}

/*
 * Allocate space for the current stack to grow before creating the
 * initial stack frame for the next coroutine.
 */
__attribute__((noinline))
static void coroutine_start(void)
{
	char stack[STACK_SIZE];

	coroutine_main(stack);
}

#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdlib.h>
#include <assert.h>

#include "longjmp.h"

static void (*real_longjmp)(jmp_buf env, int val);

__attribute__((constructor)) static void longjmp_init(void)
{
	real_longjmp = dlsym(RTLD_NEXT, "longjmp");
	assert(real_longjmp != NULL);
}

void __wrap___longjmp_chk(jmp_buf env, int val)
{
	real_longjmp(env, val);
}

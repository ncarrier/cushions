#ifndef _LONGJMP_H
#define _LONGJMP_H
#include <setjmp.h>

void __wrap___longjmp_chk(jmp_buf env, int val);

#endif /* _LONGJMP_H */

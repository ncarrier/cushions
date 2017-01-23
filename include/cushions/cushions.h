#ifndef _CUSHIONS_H
#define _CUSHIONS_H
#include <stdio.h>

#include <cushions_common.h>

CUSHIONS_API FILE *cushions_fopen(const char *path, const char *mode);
CUSHIONS_API FILE *__wrap_fopen(const char *path, const char *mode);

#endif /* _CUSHIONS_H */

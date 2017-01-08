#ifndef _CUSHIONS_H
#define _CUSHIONS_H
#include <stdio.h>

FILE *cushions_fopen(const char *path, const char *mode);
FILE *__wrap_fopen(const char *path, const char *mode);

#endif /* _CUSHIONS_H */

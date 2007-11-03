#ifndef __NXOS_UTIL_H__
#define __NXOS_UTIL_H__

#include "base/types.h"

#define MIN(x, y) ((x) < (y) ? (x): (y))
#define MAX(x, y) ((x) > (y) ? (x): (y))

void memcpy(void *dest, const void *src, U32 len);
void memset(void *dest, const U8 val, U32 len);
U32 strlen(const char *str);

U8 strncmp(const char *a, const char *b, U32 n);
U8 strcmp(const char *a, const char *b);

#endif

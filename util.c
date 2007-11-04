/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/types.h"
#include "base/util.h"

void memcpy(void *dest, const void *source, U32 len) {
  U8 *dst = (U8*)dest;
  U8 *src = (U8*)source;
  while (len--) {
    *dst++ = *src++;
  }
}

void memset(void *dest, const U8 val, U32 len) {
  U8 *dst = (U8*)dest;
  while (len--) {
    *dst++ = val;
  }
}

U32 strlen(const char *str) {
  U32 i = 0;

  while (*str++)
    i++;

  return i;
}

U32 strncmp(const char *a, const char *b, U32 n) {
  U32 i;

  for (i=0 ; i<n ; i++) {
    if (a[i] < b[i]) {
      return -1;
    } else if (a[i] > b[i]) {
      return 1;
    }
  }

  return 0;
}

U32 strcmp(const char *a, const char *b) {
  return strncmp(a, b, MIN(strlen(a), strlen(b)+1));
}

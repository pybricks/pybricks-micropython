/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/types.h"
#include "base/util.h"
#include "base/assert.h"

void memcpy(void *dest, const void *source, U32 len) {
  U8 *dst = (U8*)dest;
  U8 *src = (U8*)source;

  NX_ASSERT(dst != NULL);
  NX_ASSERT(src != NULL);

  while (len--) {
    *dst++ = *src++;
  }
}

void memset(void *dest, const U8 val, U32 len) {
  U8 *dst = (U8*)dest;

  NX_ASSERT(dst != NULL);

  while (len--) {
    *dst++ = val;
  }
}

U32 strlen(const char *str) {
  U32 i = 0;

  NX_ASSERT(str != NULL);

  while (*str++)
    i++;

  return i;
}

bool streqn(const char *a, const char *b, U32 n) {
  NX_ASSERT(a != NULL && b != NULL);

  while (n--) {
    if (*a != *b++)
      return FALSE;
    if (*a++ == '\0')
      break;
  }

  return TRUE;
}

bool streq(const char *a, const char *b) {
  NX_ASSERT(a != NULL && b != NULL);

  while (*a != '\0' && *b != '\0') {
    if (*a++ != *b++)
      return FALSE;
  }

  return *a == *b ? TRUE : FALSE;
}

char *strchr(const char *s, const char c) {
  NX_ASSERT(s != NULL);

  while (*s) {
    if (*s == c)
      return (char*)s;
    s++;
  }

  return NULL;
}

char *strrchr(const char *s, const char c) {
  const char *ptr = NULL;

  NX_ASSERT(s != NULL);

  while (*s) {
    if (*s == c)
      ptr = s;
    s++;
  }

  return (char*)ptr;
}

bool atou32(const char *s, U32* result) {
  U32 prev = 0;

  NX_ASSERT(s != NULL && result != NULL);

  *result = 0;

  // Skip leading zeros
  while (*s && *s == '0')
    s++;

  for (; *s; s++) {
    // Return 0 on invalid characters.
    if (*s < '0' || *s > '9')
      return FALSE;

    *result = (10 * *result) + (*s - '0');
    // Naive overflow check. We could do better in pure asm by
    // checking the ALU flags.
    if (*result < prev)
      return FALSE;

    prev = *result;
  }

  return TRUE;
}

bool atos32(const char *s, S32 *result) {
  S32 prev = 0;
  bool negative = FALSE;

  NX_ASSERT(s != NULL && result != NULL);

  *result = 0;

  if (*s == '-') {
    negative = TRUE;
    s++;
  }

  // Skip leading zeros
  while (*s && *s == '0')
    s++;

  for (; *s; s++) {
    // Return 0 on invalid characters.
    if (*s < '0' || *s > '9')
      return FALSE;

    *result = (10 * *result) + (*s - '0');

    // Naive overflow check. We could do better in pure asm by
    // checking the ALU flags.
    if (*result < prev)
      return FALSE;

    prev = *result;
  }

  if (negative)
    *result = -(*result);

  return TRUE;
}

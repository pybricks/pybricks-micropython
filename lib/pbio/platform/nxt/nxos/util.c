/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nxos/util.h"

bool streqn(const char *a, const char *b, uint32_t n) {
  assert(a != NULL);
  assert(b != NULL);

  while (n--) {
    if (*a != *b++)
      return false;
    if (*a++ == '\0')
      break;
  }

  return true;
}

bool streq(const char *a, const char *b) {
  assert(a != NULL);
  assert(b != NULL);

  while (*a != '\0' && *b != '\0') {
    if (*a++ != *b++)
      return false;
  }

  return *a == *b ? true : false;
}

char *strrchr (const char *s, int c) {
  const char *ptr = NULL;

  assert(s != NULL);

  while (*s) {
    if (*s == c)
      ptr = s;
    s++;
  }

  return (char*)ptr;
}

bool atou32(const char *s, uint32_t* result) {
  uint32_t prev = 0;

  assert(s != NULL && result != NULL);

  *result = 0;

  // Skip leading zeros
  while (*s && *s == '0')
    s++;

  for (; *s; s++) {
    // Return 0 on invalid characters.
    if (*s < '0' || *s > '9')
      return false;

    *result = (10 * *result) + (*s - '0');
    // Naive overflow check. We could do better in pure asm by
    // checking the ALU flags.
    if (*result < prev)
      return false;

    prev = *result;
  }

  return true;
}

bool atos32(const char *s, int32_t *result) {
  int32_t prev = 0;
  bool negative = false;

  assert(s != NULL && result != NULL);

  *result = 0;

  if (*s == '-') {
    negative = true;
    s++;
  }

  // Skip leading zeros
  while (*s && *s == '0')
    s++;

  for (; *s; s++) {
    // Return 0 on invalid characters.
    if (*s < '0' || *s > '9')
      return false;

    *result = (10 * *result) + (*s - '0');

    // Naive overflow check. We could do better in pure asm by
    // checking the ALU flags.
    if (*result < prev)
      return false;

    prev = *result;
  }

  if (negative)
    *result = -(*result);

  return true;
}

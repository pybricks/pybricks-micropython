/* Various utility functions.
 *
 * Most of these functions are defined in the libc. However, pulling
 * the libc in is quite a big hit on kernel size in some cases.
 */

#include "mytypes.h"

void memcpy(void *dest, const U8 *src, U32 len) {
  U8 *dst = (U8*)dest;
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


U32 strlen(char *str) {
  U32 i = 0;

  while (*str++)
    i++;

  return i;
}

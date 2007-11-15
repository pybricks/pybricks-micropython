/** @file util.h
 *  @brief Generally useful functions from libc.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_UTIL_H__
#define __NXOS_BASE_UTIL_H__

#include "base/types.h"

/** @addtogroup typesAndUtils */
/*@{*/

/** Return the numeric minimum of two parameters.
 *
 * @warning This macro will cause the arguments to be evaluated several
 *          times! Use only with pure arguments.
 */
#define MIN(x, y) ((x) < (y) ? (x): (y))

/** Return the numeric maximum of two parameters.
 *
 * @warning This macro will cause the arguments to be evaluated several
 *          times! Use only with pure arguments.
 */
#define MAX(x, y) ((x) > (y) ? (x): (y))

/** Copy @a len bytes from @a src to @a dest.
 *
 * @param dest Destination of the copy.
 * @param src Source of the copy.
 * @param len Number of bytes to copy.
 *
 * @warning The source and destination memory regions must not overlap.
 */
void memcpy(void *dest, const void *src, U32 len);

/** Initialize @a len bytes of @a dest with the constant @a val.
 *
 * @param dest Start of the region to initialize.
 * @param val Constant initializer to use.
 * @param len Length of the region.
 */
void memset(void *dest, const U8 val, U32 len);

/** Return the length of the given null-terminated string.
 *
 * @param str The string to evaluate.
 * @return The length in bytes of the string.
 */
U32 strlen(const char *str);

/** Order two strings based on a limited amount of input.
 *
 * @param a First string to compare.
 * @param b Second string to compare.
 * @param n Number of bytes to compare.
 * @return -1 if @a a sorts before @a b, 1 if @a b sorts before @a a, 0
 *         if both are equivalent up to @a n bytes.
 */
U32 strncmp(const char *a, const char *b, U32 n);

/** Order two strings.
 *
 * This is equivalent to @a strncmp, with the maximum length being the
 * length of the shortest input string.
 *
 * @see strncmp
 */
U32 strcmp(const char *a, const char *b);

/** Locate leftmost instance of character @a c in string @a s.
 *
 * @param s The string to search.
 * @param c The character to find.
 * @return A pointer to the first occurence of @a c in @a s, or NULL if
 * there is none.
 */
char *strchr(const char *s, const char c);

/** Locate rightmost instance of character @a c in string @a s.
 *
 * @param s The string to search.
 * @param c The character to find.
 * @return A pointer to the last occurence of @a c in @a s, or NULL if
 * there is none.
 */
char *strrchr(const char *s, const char c);

/*@}*/

#endif /* __NXOS_BASE_UTIL_H__ */

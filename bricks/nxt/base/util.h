/** @file util.h
 *  @brief Generally useful functions from libc.
 */

/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_UTIL_H__
#define __NXOS_BASE_UTIL_H__

#include "base/types.h"

#include <string.h>

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

/** Compare two string prefixes for equality.
 *
 * @param a First string to compare.
 * @param b Second string to compare.
 * @param n Number of bytes to compare.
 * @return TRUE if the first @a n bytes of @a a are equal to @a b,
 * FALSE otherwise.
 *
 * @note This function will halt on the first NULL byte it finds in
 * either string.
 */
bool streqn(const char *a, const char *b, U32 n);

/** compare two strings for equality.
 *
 * This is equivalent to @a strneq, with the maximum length being the
 * length of the shortest input string.
 *
 * @see strneq
 */
bool streq(const char *a, const char *b);

/** Convert a string to the unsigned integer it represents, if possible.
 *
 * @param s The string to convert.
 * @param result A pointer to the integer that will contain the parsed
 * result, if the conversion was successful.
 * @return TRUE with *result set correctly if the conversion was
 * successful, FALSE if the conversion failed.
 *
 * @note If the conversion fails, the value of @a *result will still
 * be clobbered, but won't contain the true value.
 */
bool atou32(const char *s, U32 *result);

/** Convert a string to the signed integer it represents, if possible.
 *
 * @param s The string to convert.
 * @param result A pointer to the integer that will contain the parsed
 * result, if the conversion was successful.
 * @return TRUE with *result set correctly if the conversion was
 * successful, FALSE if the conversion failed.
 *
 * @note If the conversion fails, the value of @a *result will still
 * be clobbered, but won't contain the true value.
 */
bool atos32(const char *s, S32 *result);

/*@}*/

#endif /* __NXOS_BASE_UTIL_H__ */

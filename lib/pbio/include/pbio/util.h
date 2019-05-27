// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#ifndef _PBIO_UTIL_H_
#define _PBIO_UTIL_H_

/**
 * Get the length of a fixed-sized array.
 * @param a     The array
 */
#define PBIO_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif // _PBIO_UTIL_H_

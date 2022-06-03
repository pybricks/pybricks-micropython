// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

/**
 * @addtogroup Version Version Information
 *
 * Library version info macros.
 *
 * @{
 */
#ifndef _PBIO_VERSION_H_
#define _PBIO_VERSION_H_

#include <pbio/util.h>

/** The current major version. */
#define PBIO_VERSION_MAJOR 3

/** The current minor version. */
#define PBIO_VERSION_MINOR 2

/** The current patch version. */
#define PBIO_VERSION_MICRO 0

/** The current prerelease level as a hex digit. */
#define PBIO_VERSION_LEVEL_HEX 0xB

/** The current prerelease serial. */
#define PBIO_VERSION_SERIAL 1

/**
 * The current prerelease level as a string.
 *
 * Also see: https://docs.python.org/3/library/sys.html#sys.version_info
 */
#if PBIO_VERSION_LEVEL_HEX == 0xA
#define PBIO_VERSION_LEVEL_STR "alpha"
#elif PBIO_VERSION_LEVEL_HEX == 0xB
#define PBIO_VERSION_LEVEL_STR "beta"
#elif PBIO_VERSION_LEVEL_HEX == 0xC
#define PBIO_VERSION_LEVEL_STR "candidate"
#elif PBIO_VERSION_LEVEL_HEX == 0xF
#define PBIO_VERSION_LEVEL_STR "final"
#else
#error "PBIO_VERSION_LEVEL_HEX must be one of 0xA, 0xB, 0xC, 0xF"
#endif

/**
 * The version as a single 32-bit integer that will be human-readable when printed as hex.
 *
 * This is useful when you need a single version number to compare to another version.
 *
 * Also see: https://docs.python.org/3/c-api/apiabiversion.html#apiabiversion
 */
#define PBIO_HEXVERSION ( \
    ((PBIO_VERSION_MAJOR & 0xFF) << 24) | \
    ((PBIO_VERSION_MINOR & 0xFF) << 16) | \
    ((PBIO_VERSION_MICRO & 0xFF) << 8) | \
    ((PBIO_VERSION_LEVEL_HEX & 0xF) << 4) | \
    (PBIO_VERSION_SERIAL & 0xF))

/** @cond INTERNAL */
#if PBIO_VERSION_LEVEL_HEX == 0xA
#define PBIO_VERSION_SUFFIX "a" PBIO_XSTR(PBIO_VERSION_SERIAL)
#elif PBIO_VERSION_LEVEL_HEX == 0xB
#define PBIO_VERSION_SUFFIX "b" PBIO_XSTR(PBIO_VERSION_SERIAL)
#elif PBIO_VERSION_LEVEL_HEX == 0xC
#define PBIO_VERSION_SUFFIX "c" PBIO_XSTR(PBIO_VERSION_SERIAL)
#else
#define PBIO_VERSION_SUFFIX
#endif
/** @endcond */

/**
 * The version as a string.
 *
 * Format is "X.Y.Z" and may have an additional prerelease suffix.
 */
#define PBIO_VERSION_STR \
    PBIO_XSTR(PBIO_VERSION_MAJOR) "." \
    PBIO_XSTR(PBIO_VERSION_MINOR) "."  \
    PBIO_XSTR(PBIO_VERSION_MICRO) PBIO_VERSION_SUFFIX

#endif // _PBIO_VERSION_H_

/** @} */

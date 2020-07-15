// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

// Common config for all Pybricks hubs/bricks

#define PYBRICKS_VERSION_MAJOR 3
#define PYBRICKS_VERSION_MINOR 0
#define PYBRICKS_VERSION_MICRO 0
#define PYBRICKS_VERSION_LEVEL_HEX 0xA
#define PYBRICKS_VERSION_SERIAL 5

// strings come from https://docs.python.org/3/library/sys.html#sys.version_info
#if PYBRICKS_VERSION_LEVEL_HEX == 0xA
#define PYBRICKS_VERSION_LEVEL_STR "alpha"
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xB
#define PYBRICKS_VERSION_LEVEL_STR "beta"
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xC
#define PYBRICKS_VERSION_LEVEL_STR "candidate"
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xF
#define PYBRICKS_VERSION_LEVEL_STR "final"
#else
#error "PYBRICKS_VERSION_LEVEL_HEX must be one of 0xA, 0xB, 0xC, 0xF"
#endif

// hex value format comes from https://docs.python.org/3/c-api/apiabiversion.html#apiabiversion
#define PYBRICKS_HEXVERSION ( \
    ((PYBRICKS_VERSION_MAJOR & 0xFF) << 24) | \
    ((PYBRICKS_VERSION_MINOR & 0xFF) << 16) | \
    ((PYBRICKS_VERSION_MICRO & 0xFF) << 8) | \
    ((PYBRICKS_VERSION_LEVEL_HEX & 0xF) << 4) | \
    (PYBRICKS_VERSION_SERIAL & 0xF))

// convert x to a string
#define _PB_CONFIG_STR(x) #x
// expand macro m and covert the result to a string
#define _PB_CONFIG_STRM(m) _PB_CONFIG_STR(m)

#if PYBRICKS_VERSION_LEVEL_HEX == 0xA
#define PYBRICKS_VERSION_SUFFIX "a" _PB_CONFIG_STRM(PYBRICKS_VERSION_SERIAL)
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xB
#define PYBRICKS_VERSION_SUFFIX "b" _PB_CONFIG_STRM(PYBRICKS_VERSION_SERIAL)
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xC
#define PYBRICKS_VERSION_SUFFIX "c" _PB_CONFIG_STRM(PYBRICKS_VERSION_SERIAL)
#else
#define PYBRICKS_VERSION_SUFFIX
#endif

#define PYBRICKS_RELEASE _PB_CONFIG_STRM(PYBRICKS_VERSION_MAJOR) "." \
    _PB_CONFIG_STRM(PYBRICKS_VERSION_MINOR) "."  _PB_CONFIG_STRM(PYBRICKS_VERSION_MICRO) \
    PYBRICKS_VERSION_SUFFIX

// Hub name
#if PYBRICKS_HUB_CITYHUB
#define PYBRICKS_HUB_NAME "cityhub"
#elif PYBRICKS_HUB_CPLUSHUB
#define PYBRICKS_HUB_NAME "cplushub"
#elif PYBRICKS_HUB_DEBUG
#define PYBRICKS_HUB_NAME "debug"
#elif PYBRICKS_HUB_EV3
#define PYBRICKS_HUB_NAME "ev3"
#elif PYBRICKS_HUB_MOVEHUB
#define PYBRICKS_HUB_NAME "movehub"
#elif PYBRICKS_HUB_NXT
#define PYBRICKS_HUB_NAME "nxt"
#elif PYBRICKS_HUB_PRIMEHUB
#define PYBRICKS_HUB_NAME "primehub"
#elif !NO_QSTR // qstr generator runs preprocessor on this file directly
#error "Unknown hub type"
#endif

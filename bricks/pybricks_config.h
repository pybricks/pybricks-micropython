// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

// Common config for all Pybricks hubs/bricks

#define PYBRICKS_VERSION_MAJOR 2
#define PYBRICKS_VERSION_MINOR 0
#define PYBRICKS_VERSION_MICRO 0
#define PYBRICKS_VERSION_LEVEL_HEX 0xA
#define PYBRICKS_VERSION_SERIAL 2

// strings come from https://docs.python.org/3/library/sys.html#sys.version_info
#if PYBRICKS_VERSION_LEVEL_HEX == 0xA
#define PYBRICKS_VERSION_LEVEL_STR "alpha"
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xB
#define PYBRICKS_VERSION_LEVEL_STR "beta"
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xC
#define PYBRICKS_VERSION_LEVEL_STR "canidate"
#elif PYBRICKS_VERSION_LEVEL_HEX == 0xF
#define PYBRICKS_VERSION_LEVEL_STR "final"
#else
#error "PYBRICKS_VERSION_LEVEL_HEX must be one of 0xA, 0xB, 0xC, 0xF"
#endif

// hex value format comes from https://docs.python.org/3/c-api/apiabiversion.html#apiabiversion
#define PYBRICKS_HEXVERSION (\
    ((PYBRICKS_VERSION_MAJOR & 0xFF) << 24) | \
    ((PYBRICKS_VERSION_MINOR & 0xFF) << 16) | \
    ((PYBRICKS_VERSION_MICRO & 0xFF) <<  8) | \
    ((PYBRICKS_VERSION_LEVEL_HEX & 0xF) <<  4) | \
    (PYBRICKS_VERSION_SERIAL & 0xF))

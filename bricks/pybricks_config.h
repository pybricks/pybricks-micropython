// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

// Common config for all Pybricks hubs/bricks

// DO NOT include this file directly, use "py/mpconfig.h" instead!

// Hub name
#if PYBRICKS_HUB_CITYHUB
#define PYBRICKS_HUB_NAME "cityhub"
#elif PYBRICKS_HUB_TECHNICHUB
#define PYBRICKS_HUB_NAME "technichub"
#elif PYBRICKS_HUB_DEBUG
#define PYBRICKS_HUB_NAME "debug"
#elif PYBRICKS_HUB_EV3BRICK
#define PYBRICKS_HUB_NAME "ev3"
#elif PYBRICKS_HUB_MOVEHUB
#define PYBRICKS_HUB_NAME "movehub"
#elif PYBRICKS_HUB_NXTBRICK
#define PYBRICKS_HUB_NAME "nxt"
#elif PYBRICKS_HUB_PRIMEHUB
#define PYBRICKS_HUB_NAME "primehub"
#elif PYBRICKS_HUB_ESSENTIALHUB
#define PYBRICKS_HUB_NAME "essentialhub"
#elif PYBRICKS_HUB_VIRTUALHUB
#define PYBRICKS_HUB_NAME "virtualhub"
#elif !NO_QSTR // qstr generator runs preprocessor on this file directly
#error "Unknown hub type"
#endif

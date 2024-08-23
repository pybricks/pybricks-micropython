// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// System capability configuration options

#ifndef _PBSYS_CONFIG_H_
#define _PBSYS_CONFIG_H_

#include "pbsysconfig.h"

#include <pbio/protocol.h>

#define PBSYS_CONFIG_APP_FEATURE_FLAGS (0 \
    + PBSYS_CONFIG_APP_BUILTIN_USER_PROGRAMS * PBIO_PYBRICKS_FEATURE_BUILTIN_USER_PROGRAMS \
    + PBSYS_CONFIG_APP_USER_PROG_FORMAT_MULTI_MPY_V6 * PBIO_PYBRICKS_FEATURE_USER_PROG_FORMAT_MULTI_MPY_V6 \
    + PBSYS_CONFIG_APP_USER_PROG_FORMAT_MULTI_MPY_V6_1_NATIVE * PBIO_PYBRICKS_FEATURE_USER_PROG_FORMAT_MULTI_MPY_V6_1_NATIVE \
    )

// When set to (1) PBSYS_CONFIG_STATUS_LIGHT indicates that a hub has a hub status light
#ifndef PBSYS_CONFIG_STATUS_LIGHT
#error "Must define PBSYS_CONFIG_STATUS_LIGHT in pbsysconfig.h"
#endif

#endif // _PBSYS_CONFIG_H_

// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PBIO_CONFIG_H_
#define _PBIO_CONFIG_H_

// This file should be defined by applications that use the PBIO library.
#include "pbioconfig.h"

#ifndef PBIO_CONFIG_ENABLE_SYS
#define PBIO_CONFIG_ENABLE_SYS (0)
#endif

// polling interval for updating servo controller
#ifndef PBIO_CONFIG_SERVO_PERIOD_MS
#define PBIO_CONFIG_SERVO_PERIOD_MS (6)
#endif

#ifndef PBIO_CONFIG_UARTDEV
#define PBIO_CONFIG_UARTDEV (0)
#endif

#endif // _PBIO_CONFIG_H_

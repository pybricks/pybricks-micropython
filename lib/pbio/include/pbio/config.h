// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PBIO_CONFIG_H_
#define _PBIO_CONFIG_H_

// This file should be defined by applications that use the PBIO library.
#include "pbioconfig.h"

#ifndef PBIO_CONFIG_ENABLE_SYS
#define PBIO_CONFIG_ENABLE_SYS (0)
#endif

// Control loop time
#ifndef PBIO_CONFIG_CONTROL_LOOP_TIME_MS
#define PBIO_CONFIG_CONTROL_LOOP_TIME_MS (5)
#endif

// Angle differentiation time window, defined as a multiple of the loop time.
// This is the time window used for calculating the average speed, so 100ms.
#define PBIO_CONFIG_DIFFERENTIATOR_WINDOW_SIZE (100 / PBIO_CONFIG_CONTROL_LOOP_TIME_MS)

// Total number of position samples to store in the differentiator buffer.
// Must be > PBIO_CONFIG_DIFFERENTIATOR_WINDOW_SIZE. This allows a user
// program to get speed with additional control over the trade off between a
// smooth but delayed value (long window) or noisy and fast value (short window).
#ifndef PBIO_CONFIG_DIFFERENTIATOR_BUFFER_SIZE
#define PBIO_CONFIG_DIFFERENTIATOR_BUFFER_SIZE (PBIO_CONFIG_DIFFERENTIATOR_WINDOW_SIZE * 3 + 1)
#endif

#define PBIO_CONFIG_NUM_DRIVEBASES (PBIO_CONFIG_SERVO_NUM_DEV / 2)

#endif // _PBIO_CONFIG_H_

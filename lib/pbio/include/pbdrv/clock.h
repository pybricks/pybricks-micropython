// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup ClockDriver Driver: Clock
 * @{
 */

#ifndef _PBDRV_CLOCK_H_
#define _PBDRV_CLOCK_H_

#include <stdint.h>

/**
 * Gets the current clock time in milliseconds.
 */
uint32_t pbdrv_clock_get_ms(void);

/**
 * Gets the current clock time in microseconds.
 */
uint32_t pbdrv_clock_get_us(void);

#endif /* _PBDRV_CLOCK_H_ */

/** @} */

// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

/**
 * @addtogroup ClockDriver Driver: Clock
 * @{
 */

#ifndef _PBDRV_CLOCK_H_
#define _PBDRV_CLOCK_H_

#include <stdint.h>

/**
 * Gets the current clock time in milliseconds (1e-3 seconds).
 */
uint32_t pbdrv_clock_get_ms(void);

/**
 * Gets the current clock time in steps of 1e-4 seconds.
 */
uint32_t pbdrv_clock_get_100us(void);

/**
 * Gets the current clock time in microseconds (1e-6 seconds).
 */
uint32_t pbdrv_clock_get_us(void);

/**
 * Busy wait delay for a very short amount of time.
 *
 * This should be used *very* sparingly and only for a few microseconds to avoid
 * blocking too long.
 *
 * @param [in]  us  The number of microseconds to delay.
 */
void pbdrv_clock_delay_us(uint32_t us);

#endif /* _PBDRV_CLOCK_H_ */

/** @} */

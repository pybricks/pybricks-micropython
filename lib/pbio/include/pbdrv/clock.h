// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

/**
 * @addtogroup ClockDriver Driver: Clock
 * @{
 */

#ifndef _PBDRV_CLOCK_H_
#define _PBDRV_CLOCK_H_

#include <stdbool.h>
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
void pbdrv_clock_busy_delay_us(uint32_t us);

/**
 * Busy wait delay for several milliseconds. May not be accurate.
 *
 * NB: Should not be used in any driver code. This exists only as a hook for
 * APIs that need a blocking delay to work when interrupts could be disabled.
 *
 * @param [in]  ms  The number of milliseconds to delay.
 */
void pbdrv_clock_busy_delay_ms(uint32_t ms);

/**
 * Tests if the millisecond clock is ticking.
 *
 * On embedded systems, this means that the clock was initialized and IRQs are
 * enabled.
 *
 * @return True if the clock is ticking, false otherwise.
 */
bool pbdrv_clock_is_ticking(void);

#endif /* _PBDRV_CLOCK_H_ */

/** @} */

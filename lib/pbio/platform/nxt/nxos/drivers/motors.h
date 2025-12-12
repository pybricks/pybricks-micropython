/** @file motors.h
 *  @brief Motor control interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_MOTORS_H__
#define __NXOS_BASE_DRIVERS_MOTORS_H__

#include <stdbool.h>
#include <stdint.h>

/** @addtogroup driver */
/*@{*/

/** @defgroup motors Motor control
 *
 * The NXT's motors have a 0 through 100% power control in both
 * directions, and features a tachometer feedback. This feedback enables
 * various control mechanisms, including positional controls.
 *
 * @note For all API calls, the motor ports are zero-indexed, ie. from 0
 * through 2, not 1 through 3.
 *
 * @note For all API calls, the rotation speed is a signed number
 * between -100 (full speed reverse) and 100 (full speed forward).
 */
/*@{*/

/** Return the internal tachometer counter value for @a motor.
 *
 * This tachometer count, if taken modulo 360, will give the angle in
 * degrees relative to the origin position. This value can be used to
 * implement more complex motor control systems, such as a PID control
 * loop.
 *
 * @param motor The motor port.
 * @return The tachometer counter value for @a motor.
 */
uint32_t nx_motors_get_tach_count(uint8_t motor);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_MOTORS_H__ */

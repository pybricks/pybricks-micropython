/** @file motors.h
 *  @brief Motor control interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_MOTORS_H__
#define __NXOS_BASE_DRIVERS_MOTORS_H__

#include "base/types.h"

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

/** Stop all rotation on @a motor, optionally applying a @a brake.
 *
 * @param motor The motor port.
 * @param brake Hard braking if TRUE, coasting stop if FALSE.
 */
void nx_motors_stop(U8 motor, bool brake);

/** Set @a motor to rotate at the given @a speed.
 *
 * @param motor The motor port.
 * @param speed The rotation speed.
 */
void nx_motors_rotate(U8 motor, S8 speed);

/** Rotate @a motor @a angle degrees at @a speed.
 *
 * @a motor The motor port.
 * @a speed The rotation speed.
 * @a angle The angle of rotation.
 * @a brake Hard braking at the end of rotation if TRUE, coasting stop
 * if FALSE.
 */
void nx_motors_rotate_angle(U8 motor, S8 speed, U32 angle, bool brake);

/** Rotate @a motor at @a speed for @a ms milliseconds.
 *
 * @a motor The motor port.
 * @a speed The rotation speed.
 * @a ms The duration of the rotation in milliseconds.
 * @a brake Hard braking at the end of rotation if TRUE, coasting stop
 * if FALSE.
 */
void nx_motors_rotate_time(U8 motor, S8 speed, U32 ms, bool brake);

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
U32 nx_motors_get_tach_count(U8 motor);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_MOTORS_H__ */

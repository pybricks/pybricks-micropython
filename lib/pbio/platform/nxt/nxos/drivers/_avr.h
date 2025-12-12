/** @file _avr.h
 *  @brief Coprocessor internal interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__AVR_H__
#define __NXOS_BASE_DRIVERS__AVR_H__

#include <stdbool.h>
#include <stdint.h>

#include "nxos/drivers/avr.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup avrinternal Coprocessor interface
 *
 * These functions allow direct access to AVR functions related to
 * sensor, motor and power control. These APIs are used by their
 * respective higher level drivers directly, and so should probably
 * not be reused directly.
 *
 * @warning Some of the functions listed here should be used with
 * care, since they may be able to damage the hardware.
 */
/*@{*/

/** Return the raw sensor value for @a sensor.
 *
 * @param sensor The sensor port, 0 through 3.
 * @return The raw sensor reading for @a sensor.
 *
 * @note This will only function correctly if @a sensor is configured in analog mode.
 */
uint32_t nx__avr_get_sensor_value(uint32_t sensor);

/** Set @a motor in motion at @a power_percent speed and @a brake braking.
 *
 * @param motor The motor port, 0 through 2.
 * @param power_percent The amount of power to apply to the motor,
 * -100 through 100.
 * @param brake Hard braking at the end of rotation if true, coasting
 * stop if false.
 *
 * @note You probably want to use the very similar public APIs in the
 * motor driver.
 */
void nx__avr_set_motor(uint32_t motor, int power_percent, bool brake);


/*@}*/
/*@}*/

#endif

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

#include "base/drivers/avr.h"

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

/** Initialize the AVR driver. */
void nx__avr_init();

/** Update the AVR state machine.
 *
 * Called every millisecond to maintain the AVR link.
 *
 * @warning Called by the systick driver when appropriate. Do @b not
 * call directly!
 */
void nx__avr_fast_update();

/** Return the raw sensor value for @a sensor.
 *
 * @param sensor The sensor port, 0 through 3.
 * @return The raw sensor reading for @a sensor.
 *
 * @note This will only function correctly if @a sensor is configured in analog mode.
 */
U32 nx__avr_get_sensor_value(U32 sensor);

/** Set @a motor in motion at @a power_percent speed and @a brake braking.
 *
 * @param motor The motor port, 0 through 2.
 * @param power_percent The amount of power to apply to the motor,
 * -100 through 100.
 * @param brake Hard braking at the end of rotation if TRUE, coasting
 * stop if FALSE.
 *
 * @note You probably want to use the very similar public APIs in the
 * motor driver.
 */
void nx__avr_set_motor(U32 motor, int power_percent, bool brake);

/** Order the AVR to power down the NXT.
 *
 * The power down occurs as soon as the command gets transferred to
 * the AVR. This function does not return, but instead goes into an
 * infinite loop after setting the command bytes, waiting to be
 * halted.
 *
 * @warning Do not use this function directly. A hard power-down of
 * the NXT can damage or confuse certain hardware. Calling
 * nx_core_halt() will take care of safely shutting down sensitive
 * drivers before powering down the brick.
 */
void nx__avr_power_down();

/** Order the AVR to put the brick in firmware update mode.
 *
 * This will reset the NXT and flash the SAM-BA bootloader to flash
 * memory, then boot back into the SAM-BA bootloader.
 *
 * @note Does not work right now. The command is ignored by the
 * coprocessor, resulting in the brick locking up.
 */
void nx__avr_firmware_update_mode();

/*@}*/
/*@}*/

#endif

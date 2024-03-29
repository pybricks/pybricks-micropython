/** @file sensors.h
 *  @brief Sensor interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_SENSORS_H__
#define __NXOS_BASE_DRIVERS_SENSORS_H__

#include <stdint.h>

#include "nxos/nxt.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup sensors Sensor interface
 *
 * The NXT sensor interface is mostly control mechanisms for analog
 * sensors (digital sensors are handled by the I2C driver). Sensors
 * have to be enabled to return readings.
 *
 * @note For all API calls, the sensor ports are zero-indexed,
 * ie. from 0 through 3, not 1 through 4.
 */
/*@{*/

/** Enumeration defining the types of sensor data pins for analog sensors. */
typedef enum {
  DIGI0 = 0,
  DIGI1,
} nx_sensors_data_pin;

/** Enable @a sensor in analog mode.
 *
 * @param sensor The sensor port.
 */
void nx_sensors_analog_enable(uint32_t sensor);

/** Get an analog reading from @a sensor.
 *
 * @param sensor The sensor port.
 * @return An uint32_t. The meaning of the value depends on the kind of
 * sensor connected.
 *
 * @note The sensor port must be enabled in analog mode.
 */
uint32_t nx_sensors_analog_get(uint32_t sensor);

/** Set the DIGI pin @a pin of @a sensor.
 *
 * @param sensor The sensor port.
 * @param pin The DIGI pin to set.
 */
void nx_sensors_analog_digi_set(uint32_t sensor, nx_sensors_data_pin pin);

/** Clear the DIGI pin @a pin of @a sensor.
 *
 * @param sensor The sensor port.
 * @param pin The DIGI pin to clear.
 */
void nx_sensors_analog_digi_clear(uint32_t sensor, nx_sensors_data_pin pin);

/** Disable the analog sensor on port @a sensor.
 *
 * @param sensor The sensor port.
 */
void nx_sensors_analog_disable(uint32_t sensor);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_SENSORS_H__ */

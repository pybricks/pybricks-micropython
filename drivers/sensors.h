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

#include "base/types.h"
#include "base/nxt.h"

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

/** Mapping of PIO ports to I2C busses.
 *
 * Each sensor port has two DIGI pins, whose use varies from sensor to
 * sensor. We remember which two pins each sensor has using the
 * sensor_pins structure.
 */
typedef struct {
  U32 scl; /**< DIGI0 - I2C clock. */
  U32 sda; /**< DIGI1 - I2C data. */
} nx_sensors_pins;

/** Enable @a sensor in analog mode.
 *
 * @param sensor The sensor port.
 */
void nx_sensors_analog_enable(U32 sensor);

/** Enable @a sensor in I2C mode.
 *
 * @param sensor The sensor port.
 */
void nx_sensors_i2c_enable(U32 sensor);

/** Get and return the port mapping for @a sensor.
 *
 * @param sensor The sensor port.
 */
const nx_sensors_pins *nx_sensors_get_pins(U32 sensor);

/** Get an analog reading from @a sensor.
 *
 * @param sensor The sensor port.
 * @return An U32. The meaning of the value depends on the kind of
 * sensor connected.
 *
 * @note The sensor port must be enabled in analog mode.
 */
U32 nx_sensors_analog_get(U32 sensor);

/** Set the DIGI pin @a pin of @a sensor.
 *
 * @param sensor The sensor port.
 * @param pin The DIGI pin to set.
 */
void nx_sensors_analog_digi_set(U32 sensor, nx_sensors_data_pin pin);

/** Clear the DIGI pin @a pin of @a sensor.
 *
 * @param sensor The sensor port.
 * @param pin The DIGI pin to clear.
 */
void nx_sensors_analog_digi_clear(U32 sensor, nx_sensors_data_pin pin);

/** Disable the sensor on port @a sensor.
 *
 * @param sensor The sensor port.
 */
void nx_sensors_disable(U32 sensor);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_SENSORS_H__ */

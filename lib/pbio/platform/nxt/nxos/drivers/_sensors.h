/** @file _sensors.h
 *  @brief Sensor internal interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__SENSORS_H__
#define __NXOS_BASE_DRIVERS__SENSORS_H__

#include <stdint.h>

#include "nxos/drivers/sensors.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup sensorsinternal Sensor interface */
/*@{*/

/** Mapping of PIO ports to I2C busses.
 *
 * Each sensor port has two DIGI pins, whose use varies from sensor to
 * sensor. We remember which two pins each sensor has using the
 * sensor_pins structure.
 */
typedef struct {
  uint32_t scl; /**< DIGI0 - I2C clock. */
  uint32_t sda; /**< DIGI1 - I2C data. */
} nx__sensors_pins;

/** Initialize the sensors driver. */
void nx__sensors_init(void);

/** Enable @a sensor in I2C mode.
 *
 * @param sensor The sensor port.
 */
void nx__sensors_i2c_enable(uint32_t sensor);

/** Get and return the port mapping for @a sensor.
 *
 * @param sensor The sensor port.
 */
const nx__sensors_pins *nx__sensors_get_pins(uint32_t sensor);

/** Disable the sensor port @a sensor.
 *
 * @param sensor The sensor port.
 *
 * @warning Make sure that all higher level drivers are "detached"
 * from the port before disabling it, or they might end up very
 * confused.
 */
void nx__sensors_disable(uint32_t sensor);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__SENSORS_H__ */

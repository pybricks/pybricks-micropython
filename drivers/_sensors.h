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

#include "base/types.h"
#include "base/drivers/sensors.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup sensorsinternal Sensor interface */
/*@{*/

/** Initialize the sensors driver. */
void nx__sensors_init();

/** Enable @a sensor in I2C mode.
 *
 * @param sensor The sensor port.
 */
void nx__sensors_i2c_enable(U32 sensor);

/** Get and return the port mapping for @a sensor.
 *
 * @param sensor The sensor port.
 */
const nx_sensors_pins *nx__sensors_get_pins(U32 sensor);

/** Disable the sensor port @a sensor.
 *
 * @param sensor The sensor port.
 *
 * @warning Make sure that all higher level drivers are "detached"
 * from the port before disabling it, or they might end up very
 * confused.
 */
void nx__sensors_disable(U32 sensor);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__SENSORS_H__ */

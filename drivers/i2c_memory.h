/** @file i2c_memory.h
 *  @brief I2C memory units driver.
 *
 * Abstraction layer for I2C remote devices acting as memory units. Some
 * digital sensors like the LEGO Ultrasonic Radar for example are simple,
 * remote memory units.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_I2C_MEMORY_H__
#define __NXOS_I2C_MEMORY_H__

#include "base/types.h"
#include "base/drivers/i2c.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup i2cmemory I2C remote memory units driver
 *
 * This driver provides an additional abstraction layer to the I2C
 * communication protocol to easily deal with I2C remote memory units.
 *
 * Some NXT sensors like the LEGO Ultrasonic sensor are designed as
 * simple memory units you can read from to retreive the readings or
 * write to it to configure the sensor.
 *
 * Being fully abstract and device-independant, this driver also be used
 * with "real" memory units.
 */
/*@{*/

/** Initializes an I2C memory unit on port @a sensor.
 *
 * @param sensor The sensor port number.
 * @param address The device I2C address.
 * @param lego_compat Defines whether the connected devices requires
 * backward I2C compatibility for LEGO sensors.
 */
void nx_i2c_memory_init(U32 sensor, U8 address, bool lego_compat);

/** Read from the memory unit connected on port @a sensor.
 *
 * @param sensor The sensor port number.
 * @param internal_address The memory address you want to read from.
 * @param buf A buffer that will hold the read data.
 * @param size The expected size of the data read from the device.
 *
 * @warning The reception buffer @a buf must be pre-allocated to hold
 * at least @a size bytes.
 *
 * @note This function is synchronous. It will wait until the underlying
 * I2C transaction(s) are done before returning.
 *
 * @return Returns the transaction status.
 */
i2c_txn_err nx_i2c_memory_read(U32 sensor, U8 internal_address,
			       U8 *buf, U32 size);

/** Write to the memory unit connected on port @a sensor.
 *
 * @param sensor The sensor port number.
 * @param internal_address The memory address you want to write to.
 * @param data The data you want to send to the device.
 * @param size The data size.
 *
 * @note This function is synchronous. It will wait until the underlying
 * I2C transaction(s) are done before returning.
 *
 * @return Returns the transaction status.
 */
i2c_txn_err nx_i2c_memory_write(U32 sensor, U8 internal_address,
				U8 *data, U32 size);

/*@}*/
/*@}*/

#endif /* __NXOS_I2C_MEMORY_H__ */

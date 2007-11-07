/** @file i2c_memory.h
 *  @brief I2C Memory units driver.
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

i2c_txn_err nx_i2c_memory_init(U32 sensor, U8 address, bool lego_compat);
i2c_txn_err nx_i2c_memory_read(U32 sensor, U8 internal_address,
			       U8 *buf, U32 size);
i2c_txn_err nx_i2c_memory_write(U32 sensor, U8 internal_address,
				U8 *data, U32 size);

#endif /* __NXOS_I2C_MEMORY_H__ */

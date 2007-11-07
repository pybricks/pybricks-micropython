/** @file _twi.h
 *  @brief Two Wire Interface driver.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__TWI_H__
#define __NXOS_BASE_DRIVERS__TWI_H__

#include "base/types.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup twi Two Wire Interface
 *
 * The TWI (Two Wire Interface) is the hardware I2C controller that
 * controls the AVR coprocessor communication bus. There are no other
 * peripherals on that bus, and no others can be added, which is why the
 * driver is internal: it is used exclusively by the AVR driver, and
 * shouldn't be messed with directly.
 *
 * @warning Messing with this driver is a bad idea! If you mess with it
 * and successfully screw up the AVR driver, the coprocessor link will
 * time out, which basically crashes half of the NXT's functionality. Be
 * careful.
 */
/*@{*/

/** Initialize the TWI driver. */
void nx__twi_init();

/** Write @a nBytes starting at @a data to I2C slave @a dev_addr.
 *
 * @param dev_addr The I2C bus address of the slave.
 * @param data Pointer to the data to write.
 * @param nBytes The amount of data to write.
 *
 * @note This function returns immediately after setting up the write
 * sequence. It does not wait until the write is effectively complete.
 */
void nx__twi_write_async(U32 dev_addr, U8 *data, U32 nBytes);

/** Read @a nBytes from I2C slave @a dev_addr into @a data.
 *
 * @param dev_addr The I2C bus address of the slave.
 * @param data Pointer to the receiving buffer.
 * @param nBytes The amount of data to read.
 *
 * @note This function returns immediately after setting up the read
 * sequence. It does not wait until the read is effectively complete.
 */
void nx__twi_read_async(U32 dev_addr, U8 *data, U32 nBytes);

/** Check the readiness of the TWI driver.
 *
 * @return TRUE if the device is idle and ready to receive commands,
 * FALSE otherwise.
 */
bool nx__twi_ready();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__TWI_H__ */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nxos/at91sam7s256.h"

#include "nxos/nxt.h"
#include "nxos/util.h"
#include "nxos/display.h"
#include "nxos/drivers/sensors.h"
#include "nxos/drivers/systick.h"
#include "nxos/drivers/i2c.h"

#include "nxos/drivers/i2c_memory.h"

/** Active waiting time before I2C transactions, in milliseconds. */
#define NX_I2C_TXN_WAIT 10

/** Initializes a remote memory unit of address 'address' on the given
 * sensor port.
 *
 * Note: if the memory unit is a LEGO radar or similar non-fully I2C
 * compliant device, set the 'lego_compat' flag to true to ensure
 * communication stability.
 */
void nx_i2c_memory_init(uint32_t sensor, uint8_t address, bool lego_compat) {
  nx_i2c_register(sensor, address, lego_compat);
}

/** Disables the remote memory unit connected on the given sensor port.
 */
void nx_i2c_memory_close(uint32_t sensor) {
  nx_i2c_unregister(sensor);
}

/** Reads a value at 'internal_address' in the memory unit on the
 * given sensor port and returns it in the given buffer. Size is the
 * expected returned data size in bytes. The buffer 'buf' should be
 * pre-allocated by the caller.
 */
i2c_txn_err nx_i2c_memory_read(uint32_t sensor, uint8_t internal_address,
			       uint8_t *buf, uint32_t size) {
  i2c_txn_err err;

  if (!buf || !size || size >= I2C_MAX_DATA_SIZE)
    return I2C_ERR_DATA;

  err = nx_i2c_start_transaction(sensor, TXN_MODE_READ,
				 &internal_address, 1, buf, size);
  if (err != I2C_ERR_OK)
    return err;

  while (nx_i2c_busy(sensor))
    nx_systick_wait_ms(NX_I2C_TXN_WAIT);

  // FIXME: Map return types correctly because cast may be wrong.
  return (i2c_txn_err) nx_i2c_get_txn_status(sensor);
}

/** Writes the given data of the given size at 'internal_address' on the
 * remote memory unit.
 */
i2c_txn_err nx_i2c_memory_write(uint32_t sensor, uint8_t internal_address,
				const uint8_t *data, uint32_t size) {
  uint8_t buf[I2C_MAX_DATA_SIZE];
  i2c_txn_err err;

  if (!data || !size || size >= I2C_MAX_DATA_SIZE)
    return I2C_ERR_DATA;

  buf[0] = internal_address;
  memcpy(buf+1, data, size);

  err = nx_i2c_start_transaction(sensor, TXN_MODE_WRITE,
				 buf, size+1, 0, 0);
  if (err != I2C_ERR_OK)
    return err;

  while (nx_i2c_busy(sensor))
    nx_systick_wait_ms(NX_I2C_TXN_WAIT);

  // FIXME: Map return types correctly because cast may be wrong.
  return (i2c_txn_err) nx_i2c_get_txn_status(sensor);
}

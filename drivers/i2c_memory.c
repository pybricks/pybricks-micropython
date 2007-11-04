/* Additional abstraction level for I2C sensors acting as I2C remote
 * memory units.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/drivers/sensors.h"
#include "base/drivers/systick.h"
#include "base/drivers/i2c.h"

#define I2C_MEMORY_READ_WAIT 10

/** Initializes a remote memory unit of address 'address' on the given
 * sensor port.
 *
 * Note: if the memory unit is a LEGO radar or similar non-fully I2C
 * compliant device, set the 'lego_compat' flag to TRUE to ensure
 * communication stability.
 */
void nx_i2c_memory_init(U8 sensor, U8 address, bool lego_compat) {
  nx_i2c_register(sensor, address, lego_compat);
}

/** Reads a value at 'internal_address' in the memory unit on the
 * given sensor port and returns it in the given buffer. Size is the
 * expected returned data size in bytes. The buffer 'buf' should be
 * pre-allocated by the caller.
 */
i2c_txn_err nx_i2c_memory_read(U8 sensor, U8 internal_address,
			       U8 *buf, U8 size) {
  i2c_txn_err err;

  if (!buf || !size || size >= I2C_MAX_DATA_SIZE)
    return I2C_ERR_DATA;

  err = nx_i2c_start_transaction(sensor, TXN_MODE_READ,
				 &internal_address, 1, buf, size);
  while (nx_i2c_busy(sensor)) {
    nx_systick_wait_ms(I2C_MEMORY_READ_WAIT);
  }

  return err;
}

/** Writes the given data of the given size at 'internal_address' on the
 * remote memory unit.
 */
i2c_txn_err nx_i2c_memory_write(U8 sensor, U8 internal_address,
				U8 *data, U8 size) {
  i2c_txn_err err;

  if (!data || !size || size >= I2C_MAX_DATA_SIZE)
    return I2C_ERR_DATA;
  
  err = nx_i2c_start_transaction(sensor, TXN_MODE_WRITE,
				 &internal_address, 1, data, size);

  while (nx_i2c_busy(sensor)) {
    nx_systick_wait_ms(I2C_MEMORY_READ_WAIT);
  }

  return err;
}

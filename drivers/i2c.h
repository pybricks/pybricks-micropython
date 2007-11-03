#ifndef __NXOS_I2C_H__
#define __NXOS_I2C_H__

#include "base/types.h"

/* The NXT does not support I2C transactions longer than
 * 16 bytes (buffer length).
 */
#define I2C_MAX_DATA_SIZE 16

typedef enum {
  I2C_ERR_OK = 0,
  I2C_ERR_UNKNOWN_SENSOR,
  I2C_ERR_NOT_READY,
  I2C_ERR_TXN_FULL,
  I2C_ERR_DATA,
} i2c_txn_err;

typedef enum
{
  TXN_MODE_WRITE = 0,
  TXN_MODE_READ,
  TXN_MODE_COUNT,
} i2c_txn_mode;

typedef enum
{
  TXN_STAT_SUCCESS = 0,
  TXN_STAT_UNKNOWN,
  TXN_STAT_IN_PROGRESS,
  TXN_STAT_FAILED,
} i2c_txn_status;

/* I2C bus control parameters. Note that in practice, on the bus,
 * a RESTART is the same as a new START bit (when running in normal bus mode).
 *
 * Pre (resp post) control values must only be used in pre_control
 * (resp post_control).
 */
typedef enum {
  I2C_CONTROL_NONE = 0,
  
  /* Pre control bits. */
  I2C_CONTROL_START,
  I2C_CONTROL_RESTART,
  
  /* Post control bits. */
  I2C_CONTROL_STOP,
} i2c_control;

void nx_i2c_init();
void nx_i2c_register(U8 sensor, U8 address, bool lego_compat);

i2c_txn_err nx_i2c_start_transaction(U8 sensor, i2c_txn_mode mode,
                                  U8 *data, U8 data_size,
                                  U8 *recv_buf, U8 recv_size);

i2c_txn_status nx_i2c_get_txn_status(U8 sensor);
bool nx_i2c_busy(U8 sensor);

#endif

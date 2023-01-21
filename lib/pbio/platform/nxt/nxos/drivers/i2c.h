/** @file i2c.h
 *  @brief I2C SoftMAC driver.
 *
 * I2C communication protocol implementation.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_I2C_H__
#define __NXOS_I2C_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup i2c I2C communication protocol SoftMAC driver
 *
 * The I2C SoftMAC driver allows I2C communication over the NXT sensor
 * ports.
 */
/*@{*/

/** Maximum transmitable data size. */
#define I2C_MAX_DATA_SIZE 16

/** I2C return codes. */
typedef enum {
  I2C_ERR_OK = 0,
  I2C_ERR_UNKNOWN_SENSOR,
  I2C_ERR_NOT_READY,
  I2C_ERR_TXN_FULL,
  I2C_ERR_DATA,
} i2c_txn_err;

/** Transaction mode definitions. */
typedef enum
{
  TXN_MODE_WRITE = 0,
  TXN_MODE_READ,
  TXN_MODE_COUNT,
} i2c_txn_mode;

/** Transaction status. */
typedef enum
{
  TXN_STAT_SUCCESS = 0,
  TXN_STAT_UNKNOWN,
  TXN_STAT_IN_PROGRESS,
  TXN_STAT_FAILED,
} i2c_txn_status;

/** I2C bus control parameters.
 *
 * Note that in practice, on the bus, a RESTART is the same as a new
 * START bit (when running in normal bus mode).
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

/** Initialize the NXT to allow I2C communication.
 */
void nx_i2c_init(void);

/** Register an I2C device of address @a address on port @a sensor.
 *
 * @param sensor The sensor port number.
 * @param address The device I2C address.
 * @param lego_compat Defines whether the device requires LEGO backward
 * I2C compatibility.
 *
 * @note If no I2C device was already registered, a call to nx_i2c_register()
 * will setup the I2C isr.
 */
void nx_i2c_register(U32 sensor, U8 address, bool lego_compat);

/** Unregister the I2C device connected on port @a sensor.
 *
 * @param sensor The sensor port number.
 *
 * @note If you're unregistering the last I2C device, the I2C isr will be
 * disabled until another device is registered.
 */
void nx_i2c_unregister(U32 sensor);

/** Perform a I2C transaction with the device on port @a sensor.
 *
 * @param sensor The sensor port number.
 * @param mode The transaction mode (reading or writing form/to the device).
 * @param data The data to send to the device. In a read transaction, the data
 * contains the command to send to the device before reading from it.
 * @param data_size The size of the data to be sent.
 * @param recv_buf A receive buffer that will hold the received data.
 * @param recv_size The expected size of the received data.
 *
 * @note This function actually creates a series of asynchronous sub
 * transactions and immediately returns. Use nx_i2c_busy() and
 * nx_i2c_get_txn_status() to track the transaction's state.
 *
 * @warning The reception buffer @a recv_buf must be pre-allocated to hold at
 * least @a recv_size bytes.
 *
 * @return This function returns an I2C error code. I2C_ERR_OK will be returned
 * if the transaction has been successfully setup. Otherwise, the appropriate
 * error codes are returned (see i2c_txn_err).
 */
i2c_txn_err nx_i2c_start_transaction(U32 sensor, i2c_txn_mode mode,
                                     const U8 *data, U32 data_size,
                                     U8 *recv_buf, U32 recv_size);

/** Get the current transaction status on port @a sensor.
 *
 * @param sensor The sensor port number.
 *
 * @warning I2C transaction return status are persistent. If no transaction
 * is processed for the given sensor, the status of the last transaction is
 * returned.
 *
 * @return Returns the state of the currently processed I2C transaction on the
 * given sensor port.
 */
i2c_txn_status nx_i2c_get_txn_status(U32 sensor);

/** Get the state of the I2C bus on port @a sensor.
 *
 * The bus is considered as busy if an I2C transaction is currently being
 * configured or processed. When all I2C sub transactions have been performed,
 * the bus is ready again.
 *
 * @note Use this function to wait for the end of a just-requested I2C
 * transaction.
 *
 * @param sensor The sensor port number.
 *
 * @return Returns the state of the I2C bus.
 */
bool nx_i2c_busy(U32 sensor);

/*@}*/
/*@}*/

#endif

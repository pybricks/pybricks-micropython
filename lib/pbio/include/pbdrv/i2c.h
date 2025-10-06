// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

/**
 * @addtogroup I2CDriver Driver
 * @{
 */

#ifndef _PBDRV_I2C_H_
#define _PBDRV_I2C_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/port.h>

typedef struct _pbdrv_i2c_dev_t pbdrv_i2c_dev_t;

#if PBDRV_CONFIG_I2C

/**
 * Get an instance of the I2C driver.
 *
 * @param [in]  id              The ID of the I2C device.
 * @param [in]  parent_process  The parent process. Allows I2C to poll process on IRQ events.
 * @param [out] i2c_dev         The I2C device.
 */
pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev);

/**
 * Does an I2C operation. To be integrated with higher-level code.
 *
 * @param [in]  state       Protothread state for async operation.
 * @param [in]  i2c_dev     The I2C device.
 * @param [in]  dev_addr    I2C device address (unshifted).
 * @param [in]  wdata       Data to be sent to the device.
 *                          Can be null if \p wlen is 0.
 * @param [in]  wlen        Length of \p wdata.
 *                          If this is 0 and \p rlen is also 0,
 *                          an empty "ping" will be sent, consisting of
 *                          only a device address (with R/nW = 0).
 *                          If this is 0 but \p rlen is not 0,
 *                          a read transaction will be sent.
 *                          If this is not 0 and \p rlen is not 0,
 *                          a write will be sent followed by a read in
 *                          a single transaction.
 * @param [out] rdata       Pointer to data read from the device. Only valid
 *                          immediately on successfull completion.
 *                          Returns null if \p rlen is 0 or the operation failed.
 * @param [in]  rlen        Size of \p rdata.
 * @param [in]  nxt_quirk   Whether to use NXT I2C transaction quirk.
 * @return                  ::PBIO_SUCCESS on success.
 */
pbio_error_t pbdrv_i2c_write_then_read(
    pbio_os_state_t *state,
    pbdrv_i2c_dev_t *i2c_dev,
    uint8_t dev_addr,
    const uint8_t *wdata,
    size_t wlen,
    uint8_t **rdata,
    size_t rlen,
    bool nxt_quirk);

#else // PBDRV_CONFIG_I2C

static inline pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    *i2c_dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_i2c_write_then_read(
    pbio_os_state_t *state,
    pbdrv_i2c_dev_t *i2c_dev,
    uint8_t dev_addr,
    const uint8_t *wdata,
    size_t wlen,
    uint8_t **rdata,
    size_t rlen,
    bool nxt_quirk) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_I2C

#endif // _PBDRV_I2C_H_

/** @} */

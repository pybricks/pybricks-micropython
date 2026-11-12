// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

/**
 * @addtogroup I2CDriver Driver
 * @{
 */

#ifndef _PBDRV_I2C_H_
#define _PBDRV_I2C_H_

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>
#include <pbio/os.h>

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
 * Configures the I/O port pins for I2C use in a platform specific way.
 *
 * Called when the port switches to I2C mode.
 *
 * @param [in]  i2c_dev     The I2C device.
 * @param [in]  scl         The SCL gpio.
 * @param [in]  sda         The SDA gpio.
 */
void pbdrv_i2c_configure_port(pbdrv_i2c_dev_t *i2c_dev, const pbdrv_gpio_t *scl, const pbdrv_gpio_t *sda);

/**
 * Undoes platform specific I2C pin setup, so the pins can be used as GPIO.
 *
 * Called when the port switches to any mode other than I2C.
 *
 * @param [in]  i2c_dev     The I2C device, or NULL.
 */
void pbdrv_i2c_release_port(pbdrv_i2c_dev_t *i2c_dev);

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

/**
 * Tests whether an I2C transaction is in progress.
 *
 * @param [in]  i2c_dev     The I2C device, or NULL.
 * @return                  True if busy, false if idle or @p i2c_dev is NULL.
 */
bool pbdrv_i2c_is_busy(pbdrv_i2c_dev_t *i2c_dev);

/**
 * Attaches a transaction buffer to the I2C device, or detaches it.
 *
 * Only for drivers without statically allocated buffers. Operations fail
 * until a buffer is attached. If the buffer is detached while a transaction
 * is in progress, it is ended cleanly at the next byte boundary.
 *
 * @param [in]  i2c_dev     The I2C device.
 * @param [in]  container   Opaque object that owns @p buffer, or NULL. It is
 *                          not used by the driver, but can be retrieved with
 *                          ::pbdrv_i2c_get_buffer_container so that multiple
 *                          users can share the same buffer.
 * @param [in]  buffer      The buffer, or NULL to detach.
 * @param [in]  size        Size of @p buffer.
 */
void pbdrv_i2c_set_buffer(pbdrv_i2c_dev_t *i2c_dev, void *container, uint8_t *buffer, size_t size);

/**
 * Gets the container of the buffer attached to the I2C device.
 *
 * @param [in]  i2c_dev     The I2C device.
 * @return                  The container, or NULL if no buffer is attached.
 */
void *pbdrv_i2c_get_buffer_container(pbdrv_i2c_dev_t *i2c_dev);

#else // PBDRV_CONFIG_I2C

static inline pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    *i2c_dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_i2c_configure_port(pbdrv_i2c_dev_t *i2c_dev, const pbdrv_gpio_t *scl, const pbdrv_gpio_t *sda) {
}

static inline void pbdrv_i2c_release_port(pbdrv_i2c_dev_t *i2c_dev) {
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

static inline bool pbdrv_i2c_is_busy(pbdrv_i2c_dev_t *i2c_dev) {
    return false;
}

static inline void pbdrv_i2c_set_buffer(pbdrv_i2c_dev_t *i2c_dev, void *container, uint8_t *buffer, size_t size) {
}

static inline void *pbdrv_i2c_get_buffer_container(pbdrv_i2c_dev_t *i2c_dev) {
    return NULL;
}

#endif // PBDRV_CONFIG_I2C

#endif // _PBDRV_I2C_H_

/** @} */

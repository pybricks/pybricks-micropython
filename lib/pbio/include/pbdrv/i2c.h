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
#include <pbio/port.h>
#include <contiki.h>

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
 * Does an I2C operation. To be replaced.
 *
 * Doesn't do anything yet. This is a placeholder so someone knowledgable with
 * EV3 I2C has a starting point without setting up all the boilerplate.
 *
 * @param [in]  i2c_dev    The I2C device.
 * @param [in]  operation  Dummy parameter.
 * @return                 ::PBIO_SUCCESS on success.
 */
pbio_error_t pbdrv_i2c_placeholder_operation(pbdrv_i2c_dev_t *i2c_dev, const char *operation);

// Delete above and add read/write functions here. See I2C driver for examples, also protothreads.

#else // PBDRV_CONFIG_I2C

static inline pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    *i2c_dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_i2c_placeholder_operation(pbdrv_i2c_dev_t *i2c_dev, const char *operation) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_I2C

#endif // _PBDRV_I2C_H_

/** @} */

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// I2C driver for EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C_EV3

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <pbdrv/gpio.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include <pbdrv/i2c.h>
#include "i2c_ev3.h"

#include "../rproc/rproc_ev3.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define debug_pr pbdrv_uart_debug_printf
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

struct _pbdrv_i2c_dev_t {
    /** Platform-specific data */
    const pbdrv_i2c_ev3_platform_data_t *pdata;
    //
    // TODO: i2c state goes here.
    //
};

static pbdrv_i2c_dev_t i2c_devs[PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES];

pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    if (id >= PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbdrv_i2c_dev_t *dev = &i2c_devs[id];
    if (!dev->pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }
    *i2c_dev = dev;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_i2c_placeholder_operation(pbdrv_i2c_dev_t *i2c_dev, const char *operation) {
    debug_pr("I2C placeholder operation %s\n", operation);
    return PBIO_SUCCESS;
}

void pbdrv_i2c_init(void) {
    for (int i = 0; i < PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES; i++) {
        const pbdrv_i2c_ev3_platform_data_t *pdata = &pbdrv_i2c_ev3_platform_data[i];
        pbdrv_i2c_dev_t *i2c = &i2c_devs[i];
        i2c->pdata = pdata;
    }
}

#endif // PBDRV_CONFIG_I2C_EV3

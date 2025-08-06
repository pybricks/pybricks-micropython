// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_I2C_EV3_H_
#define _INTERNAL_PBDRV_I2C_EV3_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <pbdrv/gpio.h>

/** Platform-specific information for I2C peripheral. */
typedef struct {
    /** GPIO pin to provide software clock. */
    pbdrv_gpio_t clk;
    /** GPIO pin to provide software data. */
    pbdrv_gpio_t sda;
} pbdrv_i2c_ev3_platform_data_t;

/**
 * Array of I2C platform data to be defined in platform.c.
 */
extern const pbdrv_i2c_ev3_platform_data_t
    pbdrv_i2c_ev3_platform_data[1];

#endif // _INTERNAL_PBDRV_I2C_EV3_H_

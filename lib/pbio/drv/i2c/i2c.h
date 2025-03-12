// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Common interface shared by I2C drivers

#ifndef _INTERNAL_PBDRV_I2C_H_
#define _INTERNAL_PBDRV_I2C_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C

/**
 * Initializes the I2C driver.
 */
void pbdrv_i2c_init(void);

#else // PBDRV_CONFIG_I2C

static inline void pbdrv_i2c_init() {
}

#endif // PBDRV_CONFIG_I2C

#endif // _INTERNAL_PBDRV_I2C_H_

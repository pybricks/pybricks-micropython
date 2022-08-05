// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for W25Qxx.

#ifndef _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_H_
#define _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_H_

#include <stdint.h>

#include "pbdrvconfig.h"

/** Platform-specific information for w25qxx flash storage. */
typedef struct {
    /** The SPI id. */
    const uint8_t spi_id;
} pbdrv_block_device_w25qxx_platform_data_t;

/**
 * Block device platform data to be defined in platform.c.
 */
extern const pbdrv_block_device_w25qxx_platform_data_t pbdrv_block_device_w25qxx_platform_data;

#endif // _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_H_

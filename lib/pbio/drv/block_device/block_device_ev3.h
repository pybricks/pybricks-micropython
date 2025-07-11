// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BLOCK_DEVICE_EV3_H_
#define _INTERNAL_PBDRV_BLOCK_DEVICE_EV3_H_

#include <stdint.h>

// Chip select pin indices
#define PBDRV_EV3_SPI0_FLASH_CS     (0)
#define PBDRV_EV3_SPI0_ADC_CS       (3)

void pbdrv_block_device_ev3_spi_tx_complete(void);
void pbdrv_block_device_ev3_spi_rx_complete(void);

#endif // _INTERNAL_PBDRV_BLOCK_DEVICE_EV3_H_

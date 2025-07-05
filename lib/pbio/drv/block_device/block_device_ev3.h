// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BLOCK_DEVICE_EV3_H_
#define _INTERNAL_PBDRV_BLOCK_DEVICE_EV3_H_

#include <stdint.h>

#include "pbdrvconfig.h"

void pbdrv_block_device_ev3_spi_tx_complete();
void pbdrv_block_device_ev3_spi_rx_complete();

// XXX The SPI flash and ADC both use the SPI0 peripheral.
// Since we do not have a fully-fledged "bus" abstraction,
// these two drivers have tight dependencies on each other.
// These functions orchestrate the connection.

int pbdrv_block_device_ev3_init_is_done();

#endif // _INTERNAL_PBDRV_BLOCK_DEVICE_EV3_H_

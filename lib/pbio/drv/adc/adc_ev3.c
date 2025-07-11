// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_EV3

#if !PBDRV_CONFIG_BLOCK_DEVICE_EV3
#error "EV3 block device driver must be enabled"
#endif

// NB: This driver is implemented by pbdrv/block_device because it is on the
// same SPI bus.

void pbdrv_adc_init(void) {
}

#endif // PBDRV_CONFIG_ADC_EV3

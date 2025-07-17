// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Driver for EV3 MMC/SD controller

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MMCSD_EV3

#include "../uart/uart_debug_first_port.h"

void pbdrv_mmcsd_init(void) {
    pbdrv_uart_debug_printf("mmcsd init\r\n");
}

#endif // PBDRV_CONFIG_MMCSD_EV3

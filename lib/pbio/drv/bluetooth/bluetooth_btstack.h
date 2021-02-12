// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Pybricks platform data for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_

#include <btstack_chipset.h>
#include <btstack_control.h>
#include <btstack_uart_block.h>

typedef struct {
    const btstack_uart_block_t *(*uart_block_instance)(void);
    const btstack_chipset_t *(*chipset_instance)(void);
    const btstack_control_t *(*control_instance)(void);
    const uint8_t *er_key;
    const uint8_t *ir_key;
} pbdrv_bluetooth_btstack_platform_data_t;

// defined in platform.c
extern const pbdrv_bluetooth_btstack_platform_data_t pbdrv_bluetooth_btstack_platform_data;

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_

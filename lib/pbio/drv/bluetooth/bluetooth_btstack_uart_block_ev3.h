// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 UART driver for BlueKitchen BTStack (stubs).

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_UART_BLOCK_EV3_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_UART_BLOCK_EV3_H_

#include <btstack_uart_block.h>
#include <pbdrv/uart.h>
#include <stdint.h>

const btstack_uart_block_t *pbdrv_bluetooth_btstack_uart_block_ev3_instance(
    void);

typedef struct {
    // The uart device connected to the Bluetooth module.
    uint8_t uart_id;
} pbdrv_bluetooth_btstack_uart_block_ev3_platform_data_t;

// To be defined in platform/ev3/platform.c
extern const pbdrv_bluetooth_btstack_uart_block_ev3_platform_data_t
    pbdrv_bluetooth_btstack_uart_block_ev3_platform_data;

#endif  // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_UART_BLOCK_EV3_H_

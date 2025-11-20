// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 UART driver for BlueKitchen BTStack. Tells btstack how to use the EV3
// UART to talk to the CC2560 Bluetooth module.

#ifndef PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_UART_BLOCK_EV3_H
#define PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_UART_BLOCK_EV3_H

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3_UART

#include <btstack_uart_block.h>
#include <pbdrv/uart.h>
#include <stdint.h>

// Returns the UART block driver instance.
const btstack_uart_block_t *pbdrv_bluetooth_btstack_uart_block_ev3_instance(
    void);

// Interrupt callbacks for UART DMA interrupts.
void pbdrv_bluetooth_btstack_uart_block_ev3_handle_tx_complete(void);
void pbdrv_bluetooth_btstack_uart_block_ev3_handle_rx_complete(void);

#endif  // PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3_UART

#endif  // PBDRV_BLUETOOTH_BLUETOOTH_BTSTACK_UART_BLOCK_EV3_H

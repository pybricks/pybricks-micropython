// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// STM32 HAL UART driver for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_UART_BLOCK_STM32_HAL_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_UART_BLOCK_STM32_HAL_H_

#include <stm32f4xx.h>
#include <btstack_uart_block.h>

const btstack_uart_block_t *pbdrv_bluetooth_btstack_uart_block_stm32_hal_instance(void);


/** BlueKitchen BTStack driver platform-specific data. */
typedef struct {
    /** UART connected to the Bluetooth chip. */
    USART_TypeDef *uart;
    /** UART transmit DMA stream. */
    DMA_Stream_TypeDef *tx_dma;
    /** UART receive DMA stream. */
    DMA_Stream_TypeDef *rx_dma;
    /** UART transmit DMA channel. */
    uint32_t tx_dma_ch;
    /** UART receive DMA channel. */
    uint32_t rx_dma_ch;
    /** UART interrupt. */
    IRQn_Type uart_irq;
    /** UART transmit DMA interrupt. */
    IRQn_Type tx_dma_irq;
    /** UART receive DMA interrupt. */
    IRQn_Type rx_dma_irq;
} pbdrv_bluetooth_btstack_uart_block_stm32_platform_data_t;

// defined in platform.c
extern const pbdrv_bluetooth_btstack_uart_block_stm32_platform_data_t
    pbdrv_bluetooth_btstack_uart_block_stm32_platform_data;

void pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_tx_dma_irq();
void pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_rx_dma_irq();
void pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_uart_irq();

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_UART_BLOCK_STM32_HAL_H_

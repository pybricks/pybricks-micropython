// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

// STM32 HAL UART driver for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_STM32_HAL_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_STM32_HAL_H_

#include <stdint.h>

#include STM32_H
#include <hci_transport.h>
#include <btstack.h>

#include <pbdrv/gpio.h>

const btstack_control_t *pbdrv_bluetooth_btstack_stm32_hal_control_instance(void);

const hci_transport_t *pbdrv_bluetooth_btstack_stm32_hal_transport_instance(void);

const void *pbdrv_bluetooth_btstack_stm32_hal_transport_config(void);

/** BlueKitchen BTStack driver platform-specific data. */
typedef struct {
    /** GPIO connected to enable pin. */
    pbdrv_gpio_t enable_gpio;
    /** UART connected to the Bluetooth chip. */
    USART_TypeDef *uart;
    #if defined(STM32H5)
    /** UART transmit DMA channel. */
    DMA_Channel_TypeDef *tx_dma;
    /** UART receive DMA channel. */
    DMA_Channel_TypeDef *rx_dma;
    /** UART transmit DMA_Request_Selection */
    uint32_t tx_dma_req;
    /** UART receive DMA_Request_Selection */
    uint32_t rx_dma_req;
    #else
    /** UART transmit DMA stream. */
    DMA_Stream_TypeDef *tx_dma;
    /** UART receive DMA stream. */
    DMA_Stream_TypeDef *rx_dma;
    /** UART transmit DMA channel. */
    uint32_t tx_dma_ch;
    /** UART receive DMA channel. */
    uint32_t rx_dma_ch;
    #endif
    /** UART interrupt. */
    IRQn_Type uart_irq;
    /** UART transmit DMA interrupt. */
    IRQn_Type tx_dma_irq;
    /** UART receive DMA interrupt. */
    IRQn_Type rx_dma_irq;
} pbdrv_bluetooth_btstack_stm32_platform_data_t;

// defined in platform.c
extern const pbdrv_bluetooth_btstack_stm32_platform_data_t
    pbdrv_bluetooth_btstack_stm32_platform_data;

void pbdrv_bluetooth_btstack_stm32_hal_handle_tx_dma_irq(void);
void pbdrv_bluetooth_btstack_stm32_hal_handle_rx_dma_irq(void);
void pbdrv_bluetooth_btstack_stm32_hal_handle_uart_irq(void);

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_STM32_HAL_H_

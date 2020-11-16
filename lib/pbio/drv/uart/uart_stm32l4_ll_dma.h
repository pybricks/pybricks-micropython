// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// UART driver for STM32L4x using DMA.

#ifndef _INTERNAL_PBDRV_UART_STM32L4_LL_DMA_H_
#define _INTERNAL_PBDRV_UART_STM32L4_LL_DMA_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <stm32l4xx.h>

/** Platform-specific information for UART peripheral. */
typedef struct {
    /** The UART Tx DMA channel registers. */
    DMA_TypeDef *tx_dma;
    /** The UART Tx DMA channel (LL_DMA_CHANNEL_x) */
    uint8_t tx_dma_ch;
    /** The UART Tx DMA request (LL_DMA_REQUEST_x) */
    uint8_t tx_dma_req;
    /** The UART Tx DMA interupt number. */
    IRQn_Type tx_dma_irq;
    /** The UART Rx DMA channel registers. */
    DMA_TypeDef *rx_dma;
    /** The UART Rx DMA channel (LL_DMA_CHANNEL_x) */
    uint8_t rx_dma_ch;
    /** The UART Rx DMA request (LL_DMA_REQUEST_x) */
    uint8_t rx_dma_req;
    /** The UART Rx DMA interupt number. */
    IRQn_Type rx_dma_irq;
    /** The UART registers. */
    USART_TypeDef *uart;
    /** The UART interupt number. */
    IRQn_Type uart_irq;
} pbdrv_uart_stm32l4_ll_dma_platform_data_t;

/**
 * Array of UART platform data to be defined in platform.c.
 */
extern const pbdrv_uart_stm32l4_ll_dma_platform_data_t
    pbdrv_uart_stm32l4_ll_dma_platform_data[PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART];

/**
 * Callback to be called by the Tx DMA IRQ handler.
 * @param id [in]   The UART instance ID.
 */
void pbdrv_uart_stm32l4_ll_dma_handle_tx_dma_irq(uint8_t id);

/**
 * Callback to be called by the Rx DMA IRQ handler.
 * @param id [in]   The UART instance ID.
 */void pbdrv_uart_stm32l4_ll_dma_handle_rx_dma_irq(uint8_t id);

/**
 * Callback to be called by the UART IRQ handler.
 * @param id [in]   The UART instance ID.
 */void pbdrv_uart_stm32l4_ll_dma_handle_uart_irq(uint8_t id);

#endif // _INTERNAL_PBDRV_UART_STM32L4_LL_DMA_H_

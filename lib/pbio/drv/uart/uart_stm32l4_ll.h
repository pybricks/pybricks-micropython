// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _UART_STM32L4_LL_H_
#define _UART_STM32L4_LL_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <stm32l4xx.h>

typedef struct {
    DMA_TypeDef *tx_dma;
    /** The UART Tx DMA channel (LL_DMA_CHANNEL_x) */
    uint8_t tx_dma_ch;
    /** The UART Tx DMA request (LL_DMA_REQUEST_x) */
    uint8_t tx_dma_req;
    IRQn_Type tx_dma_irq;
    DMA_TypeDef *rx_dma;
    /** The UART Rx DMA channel (LL_DMA_CHANNEL_x) */
    uint8_t rx_dma_ch;
    /** The UART Rx DMA request (LL_DMA_REQUEST_x) */
    uint8_t rx_dma_req;
    IRQn_Type rx_dma_irq;
    USART_TypeDef *uart;
    IRQn_Type uart_irq;
} pbdrv_uart_stm32l4_ll_platform_data_t;

extern const pbdrv_uart_stm32l4_ll_platform_data_t
    pbdrv_uart_stm32l4_ll_platform_data[PBDRV_CONFIG_UART_STM32L4_LL_NUM_UART];

void pbdrv_uart_stm32l4_ll_handle_tx_dma_irq(uint8_t id);
void pbdrv_uart_stm32l4_ll_handle_rx_dma_irq(uint8_t id);
void pbdrv_uart_stm32l4_ll_handle_uart_irq(uint8_t id);

#endif // _UART_STM32L4_LL_H_

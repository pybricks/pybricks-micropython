// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _UART_STM32L4_LL_H_
#define _UART_STM32L4_LL_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <stm32l4xx_hal.h>

typedef void (*pbdrv_uart_stm32l4_ll_dma_clear_flag_t)(DMA_TypeDef *);
typedef uint32_t (*pbdrv_uart_stm32l4_ll_dma_is_active_flag_t)(DMA_TypeDef *);

typedef struct {
    DMA_TypeDef *tx_dma;
    pbdrv_uart_stm32l4_ll_dma_clear_flag_t tx_dma_clear_tc_fn;
    pbdrv_uart_stm32l4_ll_dma_clear_flag_t tx_dma_clear_ht_fn;
    pbdrv_uart_stm32l4_ll_dma_clear_flag_t tx_dma_clear_te_fn;
    pbdrv_uart_stm32l4_ll_dma_is_active_flag_t tx_dma_is_tc_fn;
    uint8_t tx_dma_ch;
    uint8_t tx_dma_req;
    IRQn_Type tx_dma_irq;
    DMA_TypeDef *rx_dma;
    pbdrv_uart_stm32l4_ll_dma_clear_flag_t rx_dma_clear_tc_fn;
    pbdrv_uart_stm32l4_ll_dma_clear_flag_t rx_dma_clear_ht_fn;
    pbdrv_uart_stm32l4_ll_dma_is_active_flag_t rx_dma_is_tc_fn;
    pbdrv_uart_stm32l4_ll_dma_is_active_flag_t rx_dma_is_ht_fn;
    uint8_t rx_dma_ch;
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

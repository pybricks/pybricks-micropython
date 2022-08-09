// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// UART driver for STM32F4x using IRQ.

#ifndef _INTERNAL_PBDRV_UART_STM32F4_LL_IRQ_H_
#define _INTERNAL_PBDRV_UART_STM32F4_LL_IRQ_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <stm32f4xx.h>

/** Platform-specific information for UART peripheral. */
typedef struct {
    /** The UART registers. */
    USART_TypeDef *uart;
    /** The UART interrupt number. */
    IRQn_Type irq;
} pbdrv_uart_stm32f4_ll_irq_platform_data_t;

/**
 * Array of UART platform data to be defined in platform.c.
 */
extern const pbdrv_uart_stm32f4_ll_irq_platform_data_t
    pbdrv_uart_stm32f4_ll_irq_platform_data[PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART];

/**
 * Callback to be called by the UART IRQ handler.
 * @param id [in]   The UART instance ID.
 */
void pbdrv_uart_stm32f4_ll_irq_handle_irq(uint8_t id);

#endif // _INTERNAL_PBDRV_UART_STM32F4_LL_IRQ_H_

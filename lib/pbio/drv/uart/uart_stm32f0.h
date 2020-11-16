// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_UART_STM32F0_H_
#define _INTERNAL_PBDRV_UART_STM32F0_H_

#include <stdint.h>

#include "stm32f0xx.h"

typedef struct {
    USART_TypeDef *uart;
    uint8_t irq;
} pbdrv_uart_stm32f0_platform_data_t;

extern const pbdrv_uart_stm32f0_platform_data_t pbdrv_uart_stm32f0_platform_data[PBDRV_CONFIG_UART_STM32F0_NUM_UART];

void pbdrv_uart_stm32f0_handle_irq(uint8_t id);

#endif // _INTERNAL_PBDRV_UART_STM32F0_H_

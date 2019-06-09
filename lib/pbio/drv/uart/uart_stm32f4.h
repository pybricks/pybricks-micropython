// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner

#ifndef _UART_STM32F4_H_
#define _UART_STM32F4_H_

#include <stdint.h>

#include "stm32f4xx.h"

typedef struct {
    USART_TypeDef *uart;
    uint8_t irq;
} pbdrv_uart_stm32f4_platform_data_t;

extern const pbdrv_uart_stm32f4_platform_data_t pbdrv_uart_stm32f4_platform_data[PBDRV_CONFIG_UART_STM32F4_NUM_UART];

void pbdrv_uart_stm32f4_handle_irq(uint8_t id);

#endif // _UART_STM32F4_H_

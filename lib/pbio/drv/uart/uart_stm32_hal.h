// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _UART_STM32_HAL_H_
#define _UART_STM32_HAL_H_

#include <stdint.h>

#include STM32_HAL_H

typedef struct {
    USART_TypeDef *uart;
    uint8_t irq;
} pbdrv_uart_stm32_hal_platform_data_t;

extern const pbdrv_uart_stm32_hal_platform_data_t
    pbdrv_uart_stm32_hal_platform_data[PBDRV_CONFIG_UART_STM32_HAL_NUM_UART];

void pbdrv_uart_stm32_hal_handle_irq(uint8_t id);

#endif // _UART_STM32_HAL_H_

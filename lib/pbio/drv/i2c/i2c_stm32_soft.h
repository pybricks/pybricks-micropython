// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors
// Copyright (c) 2026 SparkFun Electronics

// Software (bit-banged) I2C driver for STM32 MCUs.

#ifndef _INTERNAL_PBDRV_I2C_STM32_SOFT_H_
#define _INTERNAL_PBDRV_I2C_STM32_SOFT_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C_STM32_SOFT

#include <stdint.h>

#include <pbdrv/gpio.h>

#include STM32_H

typedef struct {
    /** Basic timer that paces the bit engine on all buses. */
    TIM_TypeDef *tim;
    /** Clock rate supplied to timer (Hz). */
    uint32_t tim_clock_rate;
    /** The timer update interrupt. */
    IRQn_Type tim_irq;
} pbdrv_i2c_stm32_soft_platform_data_t;

/** Platform-specific data - defined in platform.c */
extern const pbdrv_i2c_stm32_soft_platform_data_t pbdrv_i2c_stm32_soft_platform_data;

void pbdrv_i2c_stm32_soft_handle_timer_irq(void);

#endif // PBDRV_CONFIG_I2C_STM32_SOFT

#endif // _INTERNAL_PBDRV_I2C_STM32_SOFT_H_

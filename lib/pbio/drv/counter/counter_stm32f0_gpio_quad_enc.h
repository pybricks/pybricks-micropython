// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PVDRV_COUNTER_STM32F0_GPIO_QUAD_ENC_H_
#define _PVDRV_COUNTER_STM32F0_GPIO_QUAD_ENC_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

#include <pbdrv/gpio.h>
#include "counter.h"

typedef struct {
    pbdrv_gpio_t gpio_int;
    pbdrv_gpio_t gpio_dir;
    uint8_t counter_id;
} pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t;

#if !PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC_NUM_DEV
#endif

// defined in platform/*/platform.c
extern const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t
    pbdrv_counter_stm32f0_gpio_quad_enc_platform_data[PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC_NUM_DEV];

// defined in counter_stm32f0_gpio_quad_enc.c
extern const pbdrv_counter_drv_t pbdrv_counter_stm32f0_gpio_quad_enc_drv;

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

#endif // _PVDRV_COUNTER_STM32F0_GPIO_QUAD_ENC_H_

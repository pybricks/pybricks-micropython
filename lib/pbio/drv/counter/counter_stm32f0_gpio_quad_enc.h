// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_COUNTER_STM32F0_GPIO_QUAD_ENC_H_
#define _INTERNAL_PBDRV_COUNTER_STM32F0_GPIO_QUAD_ENC_H_

#include <pbdrv/config.h>

#include <pbdrv/gpio.h>

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/counter.h>
#include <pbdrv/gpio.h>

typedef struct {
    pbdrv_gpio_t gpio_int;
    pbdrv_gpio_t gpio_dir;
    uint8_t counter_id;
} pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t;

#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

// defined in platform/*/platform.c
extern const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t
    pbdrv_counter_stm32f0_gpio_quad_enc_platform_data[PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC_NUM_DEV];

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

#endif // _INTERNAL_PBDRV_COUNTER_STM32F0_GPIO_QUAD_ENC_H_

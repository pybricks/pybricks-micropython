// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BUTTON_GPIO_H_
#define _INTERNAL_PBDRV_BUTTON_GPIO_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_GPIO

#include <pbdrv/gpio.h>
#include <pbio/button.h>

typedef struct {
    pbdrv_gpio_t gpio;
    pbdrv_gpio_pull_t pull;
    pbio_button_flags_t button;
    bool active_low;
} pbdrv_button_gpio_platform_t;

#if !PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON
#error Platform must define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON
#endif

extern const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON];

#endif // PBDRV_CONFIG_BUTTON_GPIO

#endif // _INTERNAL_PBDRV_BUTTON_GPIO_H_

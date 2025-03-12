// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#ifndef _INTERNAL_GPIO_TIAM1808_H_
#define _INTERNAL_GPIO_TIAM1808_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>

typedef struct {
    uint32_t mux_id;
    uint32_t mux_mask;
    uint32_t mux_shift;
    uint32_t gpio_bank_id;
    uint32_t gpio_mode;
} pbdrv_gpio_tiam1808_mux_t;

#endif // _INTERNAL_GPIO_TIAM1808_H_

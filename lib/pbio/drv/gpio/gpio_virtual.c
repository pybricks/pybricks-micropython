// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_GPIO_VIRTUAL

#include <stdint.h>

#include "pbdrv/gpio.h"

void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio) {
}

void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio) {
}

uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio) {
    return 0;
}

void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) {
}

void pbdrv_gpio_alt_gpio(const pbdrv_gpio_t *gpio) {
}

void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull) {
}

#endif // PBDRV_CONFIG_GPIO_VIRTUAL

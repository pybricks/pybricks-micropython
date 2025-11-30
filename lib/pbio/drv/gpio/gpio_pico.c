// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// GPIO driver using Raspberry Pi Pico SDK

#include "pbdrv/config.h"

#if PBDRV_CONFIG_GPIO_PICO

#include <stdint.h>

#include "pbdrv/gpio.h"

#include "hardware/gpio.h"

void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio) {
    gpio_put(gpio->pin, 0);
    gpio_set_dir(gpio->pin, GPIO_OUT);
}

void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio) {
    gpio_put(gpio->pin, 1);
    gpio_set_dir(gpio->pin, GPIO_OUT);
}

uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio) {
    gpio_set_dir(gpio->pin, GPIO_IN);
    return gpio_get(gpio->pin);
}

void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) {
    gpio_set_function(gpio->pin, alt);
}

void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull) {
    switch (pull) {
        case PBDRV_GPIO_PULL_NONE:
            gpio_disable_pulls(gpio->pin);
            break;
        case PBDRV_GPIO_PULL_UP:
            gpio_pull_up(gpio->pin);
            break;
        case PBDRV_GPIO_PULL_DOWN:
            gpio_pull_down(gpio->pin);
            break;
    }
}

#endif // PBDRV_CONFIG_GPIO_PICO

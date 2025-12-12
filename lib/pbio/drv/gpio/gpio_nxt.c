// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_GPIO_NXT

#include <stdint.h>

#include "pbdrv/gpio.h"

#include "at91sam7s256.h"

// Bank field is ignored for AT91SAM7S256. It is always PIOA.

#include <pbdrv/usb.h>

void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio) {
    if (!gpio) {
        return;
    }
    *AT91C_PIOA_OER = (1 << gpio->pin);
    *AT91C_PIOA_CODR = (1 << gpio->pin);
}

void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio) {
    if (!gpio) {
        return;
    }
    *AT91C_PIOA_OER = (1 << gpio->pin);
    *AT91C_PIOA_SODR = (1 << gpio->pin);
}

uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio) {
    if (!gpio) {
        return 0;
    }
    *AT91C_PIOA_ODR = (1 << gpio->pin);
    return !!(*AT91C_PIOA_PDSR & (1 << gpio->pin));
}

void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) {
    if (!gpio) {
        return;
    }
    if (alt) {
        // Enable peripheral control.
        *AT91C_PIOA_PDR = (1 << gpio->pin);
    } else {
        // Enable GPIO control.
        *AT91C_PIOA_PER = (1 << gpio->pin);
    }
}

void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull) {
    if (!gpio) {
        return;
    }
    if (pull == PBDRV_GPIO_PULL_UP) {
        *AT91C_PIOA_PPUER = (1 << gpio->pin);
    } else if (pull == PBDRV_GPIO_PULL_NONE) {
        *AT91C_PIOA_PPUDR = (1 << gpio->pin);
    } else {
        // AT91SAM7S256 does not have pull-down.
    }
}

#endif // PBDRV_CONFIG_GPIO_NXT

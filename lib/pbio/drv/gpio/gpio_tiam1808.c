// SPDX-License-Identifier: MIT
// Copyright (c) 2019 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_GPIO_TIAM1808

#include <stdint.h>

#include "pbdrv/gpio.h"

#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/gpio.h>
#include <tiam1808/psc.h>

static uint32_t get_pin_index(const pbdrv_gpio_t *gpio) {
    // TI API indexes pins from 1 to 144, so need to add 1.
    return ((uint32_t)gpio->bank) * 0x10 + gpio->pin + 1;
}

static void gpio_write(const pbdrv_gpio_t *gpio, uint8_t value) {
    if (!gpio) {
        return;
    }
    uint32_t pin_index = get_pin_index(gpio);
    GPIODirModeSet(SOC_GPIO_0_REGS, pin_index, GPIO_DIR_OUTPUT);
    GPIOPinWrite(SOC_GPIO_0_REGS, pin_index, value);
}

void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio) {
    gpio_write(gpio, GPIO_PIN_LOW);
}

void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio) {
    gpio_write(gpio, GPIO_PIN_HIGH);
}

uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio) {
    if (!gpio) {
        return 0;
    }
    uint32_t pin_index = get_pin_index(gpio);
    GPIODirModeSet(SOC_GPIO_0_REGS, pin_index, GPIO_DIR_INPUT);
    return GPIOPinRead(SOC_GPIO_0_REGS, pin_index) == GPIO_PIN_HIGH;
}

void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) {
    // Not implemented. The TI API does not make it very convenient to do this
    // programmatically. Instead, the required mode is set during init below.
}

void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull) {
}

void pbdrv_gpio_init(void) {

}

#endif // PBDRV_CONFIG_GPIO_TIAM1808

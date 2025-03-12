// SPDX-License-Identifier: MIT
// Copyright (c) 2019 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_GPIO_TIAM1808

#include <stdint.h>

#include "pbdrv/gpio.h"

#include "gpio_ev3.h"

#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/gpio.h>
#include <tiam1808/psc.h>

static uint32_t get_pin_index(const pbdrv_gpio_t *gpio) {
    // TI API indexes pins from 1 to 144, so need to add 1.
    pbdrv_gpio_ev3_mux_t *mux_info = gpio->bank;
    return mux_info->gpio_bank_id * 0x10 + gpio->pin + 1;
}

static void pbdrv_gpio_alt_gpio(const pbdrv_gpio_t *gpio) {
    if (!gpio) {
        return;
    }
    pbdrv_gpio_ev3_mux_t *mux_info = (pbdrv_gpio_ev3_mux_t *)gpio->bank;
    pbdrv_gpio_alt(gpio, mux_info->gpio_mode);
}

static void gpio_write(const pbdrv_gpio_t *gpio, uint8_t value) {
    if (!gpio) {
        return;
    }
    pbdrv_gpio_alt_gpio(gpio);
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
    pbdrv_gpio_alt_gpio(gpio);
    uint32_t pin_index = get_pin_index(gpio);
    GPIODirModeSet(SOC_GPIO_0_REGS, pin_index, GPIO_DIR_INPUT);
    return GPIOPinRead(SOC_GPIO_0_REGS, pin_index) == GPIO_PIN_HIGH;
}

void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) {
    if (!gpio) {
        return;
    }
    pbdrv_gpio_ev3_mux_t *mux_info = (pbdrv_gpio_ev3_mux_t *)gpio->bank;
    uint32_t mux = alt << mux_info->mux_shift;
    uint32_t keep = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(mux_info->mux_id)) & ~(mux_info->mux_mask);
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(mux_info->mux_id)) = (mux | keep);
}

void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull) {
    // Not implemented for TI AM1808 since EV3 does not use software pull-up/pull-down.
}

#endif // PBDRV_CONFIG_GPIO_TIAM1808

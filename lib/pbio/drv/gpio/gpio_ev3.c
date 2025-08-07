// SPDX-License-Identifier: MIT
// Copyright (c) 2019 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_GPIO_EV3

#include <stdint.h>

#include "pbdrv/gpio.h"

#include "gpio_ev3.h"

#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/gpio.h>
#include <tiam1808/psc.h>

#include "../rproc/rproc.h"
#include "../rproc/rproc_ev3.h"

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

// If the pin is not in banks 0/1, the PRU is not involved.
// Otherwise, if the PRU is initialized, do direction changes
// via the PRU in order to prevent race conditions.
// If the PRU is not initialized (i.e. doing direction changes
// during early boot), also set the direction directly via
// the ARM. The PRU code polls these direction change registers
// continuously and is not expected to block for too long,
// only on the order of microseconds.
#define PBDRV_GPIO_EV3_ARM_OWNS_GPIO_BANK(pin_index)    \
    ((pin_index) > 32 || !pbdrv_rproc_is_ready())

static void gpio_write(const pbdrv_gpio_t *gpio, uint8_t value) {
    if (!gpio) {
        return;
    }
    pbdrv_gpio_alt_gpio(gpio);
    uint32_t pin_index = get_pin_index(gpio);
    if (PBDRV_GPIO_EV3_ARM_OWNS_GPIO_BANK(pin_index)) {
        GPIODirModeSet(SOC_GPIO_0_REGS, pin_index, GPIO_DIR_OUTPUT);
    } else if (GPIODirModeGet(SOC_GPIO_0_REGS, pin_index) != GPIO_DIR_OUTPUT) {
        uint32_t val = 1 << (pin_index - 1);
        pbdrv_rproc_ev3_pru1_shared_ram.gpio_bank_01_dir_clr = val;
        while (pbdrv_rproc_ev3_pru1_shared_ram.gpio_bank_01_dir_clr) {
            // Wait for the PRU to process the command
        }
    }
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
    if (PBDRV_GPIO_EV3_ARM_OWNS_GPIO_BANK(pin_index)) {
        GPIODirModeSet(SOC_GPIO_0_REGS, pin_index, GPIO_DIR_INPUT);
    } else if (GPIODirModeGet(SOC_GPIO_0_REGS, pin_index) != GPIO_DIR_INPUT) {
        uint32_t val = 1 << (pin_index - 1);
        pbdrv_rproc_ev3_pru1_shared_ram.gpio_bank_01_dir_set = val;
        while (pbdrv_rproc_ev3_pru1_shared_ram.gpio_bank_01_dir_set) {
            // Wait for the PRU to process the command
        }
    }
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

#endif // PBDRV_CONFIG_GPIO_EV3

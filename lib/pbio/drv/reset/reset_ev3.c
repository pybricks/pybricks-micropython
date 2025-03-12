// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

// Manages power off and reset for EV3 with TIAM1808.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_EV3

#include <pbdrv/reset.h>
#include <pbdrv/gpio.h>

#include "../drv/gpio/gpio_ev3.h"

#include <tiam1808/hw/hw_syscfg0_AM1808.h>

static const pbdrv_gpio_t poweroff_pin = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 13,
        .mux_mask = SYSCFG_PINMUX13_PINMUX13_19_16,
        .mux_shift = SYSCFG_PINMUX13_PINMUX13_19_16_SHIFT,
        .gpio_bank_id = 6,
        .gpio_mode = SYSCFG_PINMUX13_PINMUX13_19_16_GPIO6_11,
    },
    .pin = 11
};

void pbdrv_reset_init(void) {
    pbdrv_gpio_alt(&poweroff_pin, ((pbdrv_gpio_tiam1808_mux_t *)poweroff_pin.bank)->gpio_mode);
}

void pbdrv_reset(pbdrv_reset_action_t action) {
    for (;;) {
        switch (action) {
            case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
                // TODO
                break;
            // TODO: implement case PBDRV_RESET_ACTION_RESET
            default:
                break;
        }
    }
}

pbdrv_reset_reason_t pbdrv_reset_get_reason(void) {
    return PBDRV_RESET_REASON_NONE;
}

void pbdrv_reset_power_off(void) {
    pbdrv_gpio_out_low(&poweroff_pin);
}

#endif // PBDRV_CONFIG_RESET_EV3

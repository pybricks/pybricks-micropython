// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

// Manages power off and reset for EV3 with TIAM1808.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_EV3

#include <pbdrv/reset.h>
#include <pbdrv/gpio.h>

void pbdrv_reset_init(void) {

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
    const pbdrv_gpio_t power_pin = {
        .bank = (void *)6,
        .pin = 11,
    };
    pbdrv_gpio_out_low(&power_pin);
}

#endif // PBDRV_CONFIG_RESET_EV3

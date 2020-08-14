// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Manages power off and reset for LEGO MINDSTORMS NXT.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_NXT

#include <nxt/nxt_avr.h>
#include <nxt/nxt_lcd.h>

#include <pbdrv/reset.h>

void pbdrv_reset(pbdrv_reset_action_t action) {
    nxt_lcd_enable(0);
    for (;;) {
        switch (action) {
            case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
                nxt_avr_firmware_update_mode();
                break;
            // TODO: implement case PBDRV_RESET_ACTION_RESET
            default:
                nxt_avr_power_down();
                break;
        }
    }
}

#endif // PBDRV_CONFIG_RESET_NXT

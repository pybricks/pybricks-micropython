// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement user program start/stop from button
// TODO: implement status light indications
// TODO: implement additional buttons and Bluetooth light for SPIKE Prime
// TODO: implement additional buttons and menu system (via screen) for NXT

#include <pbdrv/reset.h>
#include <pbio/button.h>
#include <pbsys/status.h>

/**
 * Polls the HMI.
 *
 * This is called periodically to update the current HMI state.
 */
void pbsys_hmi_poll() {
    pbio_button_flags_t btn;
    pbio_button_is_pressed(&btn);

    if (btn & PBIO_BUTTON_CENTER) {
        pbsys_status_set(PBSYS_STATUS_POWER_BUTTON_PRESSED);
        // power off when button is held down for 3 seconds
        if (pbsys_status_test_debounce(PBSYS_STATUS_POWER_BUTTON_PRESSED, true, 3000)) {
            pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
        }
    } else {
        pbsys_status_clear(PBSYS_STATUS_POWER_BUTTON_PRESSED);
    }
}

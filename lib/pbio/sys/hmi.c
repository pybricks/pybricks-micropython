// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement user program start/stop from button
// TODO: implement additional buttons and Bluetooth light for SPIKE Prime
// TODO: implement additional buttons and menu system (via screen) for NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/core.h>
#include <pbdrv/reset.h>
#include <pbdrv/led.h>
#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/light.h>
#include <pbsys/config.h>
#include <pbsys/status.h>

#include "light_matrix.h"
#include "light.h"

void pbsys_hmi_init(void) {
    pbsys_status_light_init();
    pbsys_hub_light_matrix_init();
}

void pbsys_hmi_handle_event(process_event_t event, process_data_t data) {
    pbsys_status_light_handle_event(event, data);
    pbsys_hub_light_matrix_handle_event(event, data);
}

/**
 * Polls the HMI.
 *
 * This is called periodically to update the current HMI state.
 */
void pbsys_hmi_poll(void) {
    pbio_button_flags_t btn;
    pbio_button_is_pressed(&btn);

    if (btn & PBIO_BUTTON_CENTER) {
        pbsys_status_set(PBSYS_STATUS_POWER_BUTTON_PRESSED);
        // power off when button is held down for 3 seconds
        if (pbsys_status_test_debounce(PBSYS_STATUS_POWER_BUTTON_PRESSED, true, 3000)) {
            // TODO: need to do shutdown sequence here - play animation, sound, etc.
            // then make sure all non-driver contiki processes are stopped so they
            // don't try to use any drivers during pbdrv_deinit().
            pbdrv_deinit();
            pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
        }
    } else {
        pbsys_status_clear(PBSYS_STATUS_POWER_BUTTON_PRESSED);
    }

    pbsys_status_light_poll();
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

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
#include <pbio/event.h>
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

    #if PBSYS_CONFIG_BATTERY_CHARGER
    // On the Technic Large hub, USB can keep the power on even though we are
    // "shutdown", so if the button is pressed again, we reset to turn back on
    if (
        pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)
        && event == PBIO_EVENT_STATUS_SET
        && (pbio_pybricks_status_t)data == PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED
        ) {
        pbdrv_reset(PBDRV_RESET_ACTION_RESET);
    }
    #endif // PBSYS_CONFIG_BATTERY_CHARGER
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
        pbsys_status_set(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
        // power off when button is held down for 3 seconds
        if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED, true, 3000)) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN);
        }
    } else {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
    }

    pbsys_status_light_poll();
}

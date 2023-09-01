// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement additional buttons for SPIKE Prime
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
#include <pbsys/bluetooth.h>
#include <pbsys/config.h>
#include <pbsys/status.h>

#include "light_matrix.h"
#include "light.h"
#include "program_load.h"

static struct pt user_program_start_pt;

/**
 * Protothread to monitor the button state to trigger starting the user program.
 * @param [in]  button_pressed      The current button state.
 */
static PT_THREAD(user_program_start(bool button_pressed)) {
    struct pt *pt = &user_program_start_pt;
    // HACK: Misuse of protothread to reduce code size. This is the same
    // as checking if the user program is running after each PT_WAIT.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        goto start;
    }

    PT_BEGIN(pt);

    for (;;) {
    start:
        // button may still be pressed from power on or user program stop
        PT_WAIT_UNTIL(pt, !button_pressed);
        PT_WAIT_UNTIL(pt, button_pressed);
        PT_WAIT_UNTIL(pt, !button_pressed);

        // if we made it through a full press and release, without the user
        // program running, then start the user program
        pbsys_program_load_start_user_program();
    }

    PT_END(pt);
}

void pbsys_hmi_init(void) {
    pbsys_status_light_init();
    pbsys_hub_light_matrix_init();
    PT_INIT(&user_program_start_pt);
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
    bool bluetooth_disabled;

    static bool bluetooth_toggled = false;

    if (pbio_button_is_pressed(&btn) == PBIO_SUCCESS) {
        if (btn & PBIO_BUTTON_CENTER) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
            user_program_start(true);

            // power off when button is held down for 3 seconds
            if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED, true, 3000)) {
                pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
            }
        } else if ((btn & PBIO_BUTTON_RIGHT_UP) &&
                   !pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_BLUETOOTH_BUTTON_PRESSED);

            if (!bluetooth_toggled &&
                pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BLUETOOTH_BUTTON_PRESSED, true, 50)) {
                bluetooth_toggled = true;
                bluetooth_disabled = !pbsys_program_load_get_bluetooth_disabled();
                pbsys_program_load_set_bluetooth_disabled(bluetooth_disabled);
            }
        } else {
            pbsys_status_clear(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
            user_program_start(false);

            pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLUETOOTH_BUTTON_PRESSED);
            bluetooth_toggled = false;
        }
    }

    pbsys_status_light_poll();
}

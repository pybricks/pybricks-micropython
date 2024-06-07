// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement additional buttons and menu system (via matrix display) for SPIKE Prime
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
#include <pbsys/storage_settings.h>

#include "light_matrix.h"
#include "light.h"
#include "user_program.h"

static struct pt update_program_run_button_wait_state_pt;

/**
 * Protothread to monitor the button state to trigger starting the user program.
 * @param [in]  button_pressed      The current button state.
 */
static PT_THREAD(update_program_run_button_wait_state(bool button_pressed)) {
    struct pt *pt = &update_program_run_button_wait_state_pt;
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
        pbsys_user_program_start_program();
    }

    PT_END(pt);
}

#if PBSYS_CONFIG_BLUETOOTH_TOGGLE

static struct pt update_bluetooth_button_wait_state_pt;

/**
 * Protothread to monitor the button state to toggle Bluetooth.
 * @param [in]  button_pressed      The current button state.
 */
static PT_THREAD(update_bluetooth_button_wait_state(bool button_pressed)) {
    struct pt *pt = &update_bluetooth_button_wait_state_pt;
    // HACK: Misuse of protothread to reduce code size. This is the same
    // as checking if the user program is running after each PT_WAIT.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        goto start;
    }

    PT_BEGIN(pt);

    for (;;) {
    start:
        // button may still be pressed during user program
        PT_WAIT_UNTIL(pt, !button_pressed);
        PT_WAIT_UNTIL(pt, button_pressed);
        pbsys_storage_settings_bluetooth_enabled_request_toggle();
    }

    PT_END(pt);
}

#endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE

void pbsys_hmi_init(void) {
    pbsys_status_light_init();
    pbsys_hub_light_matrix_init();
    PT_INIT(&update_program_run_button_wait_state_pt);

    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    PT_INIT(&update_bluetooth_button_wait_state_pt);
    #endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE
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

    if (pbio_button_is_pressed(&btn) == PBIO_SUCCESS) {
        if (btn & PBIO_BUTTON_CENTER) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
            update_program_run_button_wait_state(true);

            // power off when button is held down for 2 seconds
            if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED, true, 2000)) {
                pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
            }
        } else {
            pbsys_status_clear(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
            update_program_run_button_wait_state(false);
        }

        #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
        update_bluetooth_button_wait_state(btn & PBSYS_CONFIG_BLUETOOTH_TOGGLE_BUTTON);
        #endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE
    }

    pbsys_status_light_poll();
}

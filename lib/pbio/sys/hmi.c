// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement additional buttons and menu system (via matrix display) for SPIKE Prime
// TODO: implement additional buttons and menu system (via screen) for NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>
#include <pbdrv/core.h>
#include <pbdrv/reset.h>
#include <pbdrv/led.h>
#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/light.h>
#include <pbio/os.h>
#include <pbsys/config.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/storage_settings.h>

#include "light_matrix.h"
#include "light.h"

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
#else
#define DEBUG_PRINT(...)
#endif

// The selected slot is not persistent across reboot, so that the first slot
// is always active on boot. This allows consistently starting programs without
// visibility of the display.
static uint8_t selected_slot = 0;

/**
 * Gets the currently selected program slot.
 *
 * @return The currently selected program slot (zero-indexed).
 */
uint8_t pbsys_hmi_get_selected_program_slot(void) {
    return selected_slot;
}

void pbsys_hmi_init(void) {
    pbsys_status_light_init();
    pbsys_hub_light_matrix_init();
}

void pbsys_hmi_handle_status_change(pbsys_status_change_t event, pbio_pybricks_status_t data) {
    pbsys_status_light_handle_status_change(event, data);
}

/**
 * Polls the HMI.
 *
 * This is called periodically to update the current HMI state.
 */
void pbsys_hmi_poll(void) {
    pbio_button_flags_t btn = pbdrv_button_get_pressed();

    if (btn & PBIO_BUTTON_CENTER) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);

        // power off when button is held down for 2 seconds
        if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED, true, 2000)) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
        }
    } else {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
    }

    pbsys_status_light_poll();
}

/**
 * Registers button presses to update the visual UI state and request the
 * launch of a program.
 */
static pbio_error_t pbsys_hmi_launch_program_with_button(pbio_os_state_t *state) {

    pbio_button_flags_t pressed;

    PBIO_OS_ASYNC_BEGIN(state);

    pbsys_hub_light_matrix_update_program_slot();

    for (;;) {

        // Buttons could be pressed at the end of the user program, so wait for
        // a release and then a new press.
        PBIO_OS_AWAIT_WHILE(state, pressed = pbdrv_button_get_pressed());
        PBIO_OS_AWAIT_UNTIL(state, (pressed = pbdrv_button_get_pressed()) || pbsys_main_program_start_is_requested());

        // Abandon this UI thread if a program was scheduled to start from the host.
        if (pbsys_main_program_start_is_requested()) {
            break;
        }

        if (pressed & PBIO_BUTTON_CENTER) {
            pbio_error_t err = pbsys_main_program_request_start(selected_slot, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);

            if (err == PBIO_SUCCESS) {
                // Program is available so we can leave this UI thread and
                // start it. First wait for all buttons to be released so the
                // user doesn't accidentally push their robot off course.
                PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
                break;
            }

            // TODO: Show brief visual indicator if program not available.
        }

        // On right, increment slot when possible.
        if ((pressed & PBIO_BUTTON_RIGHT) && selected_slot < 4) {
            selected_slot++;
            pbsys_hub_light_matrix_update_program_slot();
        }

        // On left, decrement slot when possible.
        if ((pressed & PBIO_BUTTON_LEFT) && selected_slot > 0) {
            selected_slot--;
            pbsys_hub_light_matrix_update_program_slot();
        }
    }

    // Wait for all buttons to be released so the user doesn't accidentally
    // push their robot off course. This await also makes it safe to call this
    // function after completion. We just exit from here right away again.
    PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#if PBSYS_CONFIG_BLUETOOTH_TOGGLE
static bool pbsys_hmi_bluetooth_button_is_pressed() {
    return pbdrv_button_get_pressed() & PBSYS_CONFIG_BLUETOOTH_TOGGLE_BUTTON;
}
#else
static inline bool pbsys_hmi_bluetooth_button_is_pressed() {
    return false;
}
#endif

static pbio_error_t pbsys_hmi_monitor_bluetooth_state(pbio_os_state_t *state) {

    PBIO_OS_ASYNC_BEGIN(state);

    // FIXME: We can drop this once Bluetooth poweron-init is moved to pbdrv.
    // It waits for Bluetooth to be ready before staring pbsys.
    static pbio_os_timer_t timer;
    if (pbdrv_clock_get_ms() < 1000) {
        PBIO_OS_AWAIT_MS(state, &timer, 2000);
    }

    for (;;) {

        // No need to monitor buttons if connected, so just wait for program
        // start or disconnect and then re-assess.
        if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
            PBIO_OS_AWAIT_UNTIL(state,
                pbsys_main_program_start_is_requested() ||
                !pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)
                );
            if (pbsys_main_program_start_is_requested()) {
                // Done, ready to run the program.
                break;
            } else {
                // Disconnected, so start over.
                continue;
            }
        }

        // Begin advertising.
        pbdrv_bluetooth_start_advertising();
        pbsys_storage_settings_bluetooth_enabled_set(true);
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
        pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);

        // Wait for connection, program run, or bluetooth toggle.
        PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
        PBIO_OS_AWAIT_UNTIL(state,
            pbsys_hmi_bluetooth_button_is_pressed() ||
            pbsys_main_program_start_is_requested() ||
            pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)
            );

        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
        if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)) {
            // On connecting we can stop monitoring the button. Advertising stops
            // automatically.
            continue;
        }
        pbdrv_bluetooth_stop_advertising();

        if (pbsys_main_program_start_is_requested()) {
            // Done, ready to run the program.
            break;
        }

        // Otherwise, we got here because the Bluetooth button was toggled.
        pbsys_storage_settings_bluetooth_enabled_set(false);

        // Bluetooth is now off so we only have to wait for another button
        // press or a program started with the buttons.
        PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
        PBIO_OS_AWAIT_UNTIL(state,
            pbsys_hmi_bluetooth_button_is_pressed() ||
            pbsys_main_program_start_is_requested()
            );
        if (pbsys_main_program_start_is_requested()) {
            // Done, ready to run the program.
            break;
        }
    }

    // Wait for all buttons to be released before starting under all conditions.
    PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}
/**
 * Drives all processes while we wait for user input. This completes when a
 * user program request is made using the buttons or by a connected host.
 */
void pbsys_hmi_await_program_selection(void) {
    pbio_os_state_t btn_state = 0;
    pbio_os_state_t ble_state = 0;

    pbio_error_t btn_err;
    pbio_os_state_t ble_err;

    do {
        btn_err = pbsys_hmi_launch_program_with_button(&btn_state);
        ble_err = pbsys_hmi_monitor_bluetooth_state(&ble_state);
        pbio_os_run_processes_and_wait_for_event();
    } while ((btn_err == PBIO_ERROR_AGAIN || ble_err == PBIO_ERROR_AGAIN) && !pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST));
}

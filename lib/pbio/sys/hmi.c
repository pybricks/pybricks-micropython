// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement additional buttons and menu system (via matrix display) for SPIKE Prime
// TODO: implement additional buttons and menu system (via screen) for NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
#include <pbsys/host.h>
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

/**
 * Registers button presses to update the visual UI state and request the
 * launch of a program.
 *
 * NB: Must allow calling after completion, so must set os state prior to returning.
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

            pbio_error_t err = pbsys_main_program_request_start(pbsys_status_get_selected_slot(), PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);

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
        if (pressed & PBIO_BUTTON_RIGHT) {
            pbsys_status_increment_selected_slot(true);
            pbsys_hub_light_matrix_update_program_slot();
        }

        // On left, decrement slot when possible.
        if (pressed & PBIO_BUTTON_LEFT) {
            pbsys_status_increment_selected_slot(false);
            pbsys_hub_light_matrix_update_program_slot();
        }
    }

    // Wait for all buttons to be released so the user doesn't accidentally
    // push their robot off course. This await also makes it safe to call this
    // function after completion. We just exit from here right away again.
    PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Monitors Bluetooth enable button and starts/stops advertising as needed.
 *
 * NB: Must allow calling after completion, so must set os state prior to returning.
 */
static pbio_error_t pbsys_hmi_monitor_bluetooth_state(pbio_os_state_t *state) {

    #if !PBSYS_CONFIG_BLUETOOTH
    return PBIO_ERROR_NOT_SUPPORTED;
    #endif

    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    bool bluetooth_button_is_pressed = pbdrv_button_get_pressed() & PBSYS_CONFIG_BLUETOOTH_TOGGLE_BUTTON;
    #else
    bool bluetooth_button_is_pressed = false;
    #endif

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        // No need to monitor Bluetooth button if connected, so just wait for
        // program start or disconnect.
        if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
            PBIO_OS_AWAIT_UNTIL(state,
                pbsys_main_program_start_is_requested() ||
                !pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)
                );
            if (pbsys_main_program_start_is_requested()) {
                // Done, ready to run the program.
                break;
            }
        }

        // Start with Bluetooth off.
        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_power_on(&sub, false));

        // Since bluetooth is off, we just have to wait for a program start
        // with the buttons or until Bluetooth is enabled with the button or as
        // loaded from settings.
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
        PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
        PBIO_OS_AWAIT_UNTIL(state,
            pbsys_storage_settings_bluetooth_enabled_get() ||
            bluetooth_button_is_pressed ||
            pbsys_main_program_start_is_requested()
            );
        if (pbsys_main_program_start_is_requested()) {
            // Done, ready to run the program.
            break;
        }

        // Enable bluetooth and begin advertising.
        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_power_on(&sub, true));
        pbdrv_bluetooth_start_advertising(true);
        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_await_advertise_or_scan_command(&sub, NULL));

        pbsys_storage_settings_bluetooth_enabled_set(true);
        pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);

        // Wait for connection, program run, or bluetooth toggle.
        PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
        PBIO_OS_AWAIT_UNTIL(state,
            bluetooth_button_is_pressed ||
            pbsys_main_program_start_is_requested() ||
            pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)
            );

        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
        if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)) {
            // On connecting we can stop monitoring the button. Advertising stops
            // automatically.
            continue;
        }

        pbdrv_bluetooth_start_advertising(false);
        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_await_advertise_or_scan_command(&sub, NULL));

        if (pbsys_main_program_start_is_requested()) {
            // Done, ready to run the program.
            break;
        }

        // Otherwise, we got here because the Bluetooth button was toggled.
        pbsys_storage_settings_bluetooth_enabled_set(false);
    }

    // Wait for all buttons to be released before starting under all conditions.
    PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed());
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Drives all processes while we wait for user input. This completes when a
 * user program request is made using the buttons or by a connected host.
 *
 * @return  Error code.
 *          ::PBIO_SUCCESS when a program is selected.
 *          ::PBIO_ERROR_CANCELED when selection was cancelled by shutdown request.
 *          ::PBIO_ERROR_TIMEDOUT when there was no user interaction for a long time.
 */
pbio_error_t pbsys_hmi_await_program_selection(void) {

    #if PBSYS_CONFIG_USER_PROGRAM_AUTO_START
    // Skip any UI, always just start the REPL except on shutdown.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
        return PBIO_ERROR_CANCELED;
    }
    pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_REPL, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
    return PBIO_SUCCESS;
    #endif

    pbio_os_state_t btn_state = 0;
    pbio_os_state_t ble_state = 0;

    pbio_error_t btn_err;
    pbio_os_state_t ble_err;

    uint32_t time_start = pbdrv_clock_get_ms();

    do {
        // Shutdown may be requested by a background process such as critical
        // battery or user interaction. This means we should skip the user
        // program so return an error.
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }

        // Don't time out while connected to host.
        if (pbsys_host_is_connected()) {
            time_start = pbdrv_clock_get_ms();
        }

        // Timeout on idle.
        if (pbdrv_clock_get_ms() - time_start > 3 * 60000) {
            return PBIO_ERROR_TIMEDOUT;
        }

        // Iterate UI once. We will be back here on the next event, including
        // on the next timer tick.
        btn_err = pbsys_hmi_launch_program_with_button(&btn_state);
        ble_err = pbsys_hmi_monitor_bluetooth_state(&ble_state);

        // run all processes and wait for next event.
        pbio_os_run_processes_and_wait_for_event();

    } while ((btn_err == PBIO_ERROR_AGAIN || ble_err == PBIO_ERROR_AGAIN));

    return PBIO_SUCCESS;
}

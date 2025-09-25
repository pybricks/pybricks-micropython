// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user for Powered Up
// hubs with BLE, lights, and one or more buttons.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_PUP

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pbdrv/bluetooth.h>

#include <pbio/button.h>
#include <pbio/os.h>
#include <pbsys/host.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/storage_settings.h>

#include "hmi.h"
#include "light_matrix.h"

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
#else
#define DEBUG_PRINT(...)
#endif

/**
 * The HMI is a loop running the following steps:
 *
 *    - Update Bluetooth state, based on current state. This enables or
 *      disables bluetooth and starts/stop advertising.
 *    - Wait for any buttons to be released in case they were pressed
 *    - Wait for a button press, external program start, or connection change.
 *    - If valid program requested, break out of loop to start program. Otherwise,
 *      update state based on what happened and start over.
 *    - After leaving the loop, wait for all buttons to be released.
 *
 * The three waiting operations are cancelled if poweroff is requested.
 */
static pbio_error_t run_ui(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    static pbio_os_state_t sub;

    /**
     * Persistent state indicating whether we were connected the last time the
     * HMI ran. If it was connected before but not now, we know we became
     * disconnected. This is when we restart Bluetooth to get a new address and
     * avoid reconnection issues. We also want to do that on boot, so we start
     * this in true.
     */
    static bool previously_connected = true;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        DEBUG_PRINT("Start HMI loop\n");

        // Visually indicate current state on supported hubs.
        pbsys_hub_light_matrix_update_program_slot();

        // Initialize Bluetooth depending on current state.
        if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)) {
            DEBUG_PRINT("Connected: yes\n");
            pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
            pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
            previously_connected = true;
            // No need to stop advertising since this is automatic.
        } else {
            // Not connected right now.
            DEBUG_PRINT("Connected: No\n");
            if (previously_connected) {
                // Became disconnected just now or some time throughout the
                // last user program run. Reset Bluetooth to get a new address.
                // Also used the very first time we power on.
                DEBUG_PRINT("Reset Bluetooth.\n");
                PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_power_on(&sub, false));
            }
            pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
            previously_connected = false;

            // Enable or disable Bluetooth depending on user setting. This is
            // a safe no-op if this was already set.
            DEBUG_PRINT("Bluetooth is configured to be: %s. \n", pbsys_storage_settings_bluetooth_enabled_get() ? "on" : "off");
            PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_power_on(&sub, pbsys_storage_settings_bluetooth_enabled_get()));

            // Update advertising state.
            if (pbsys_storage_settings_bluetooth_enabled_get()) {
                // Start advertising if we aren't already.
                if (!pbsys_status_test(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING)) {
                    pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
                    DEBUG_PRINT("Start advertising.\n");
                    pbdrv_bluetooth_start_advertising(true);
                    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_await_advertise_or_scan_command(&sub, NULL));
                }
            } else {
                // Not advertising if Bluetooth is disabled. The physical state
                // is already off, but we need the blinking to stop too.
                pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
            }
        }

        // Buttons could be pressed at the end of the user program, so wait for
        // a release and then a new press, or until we have to exit early.
        DEBUG_PRINT("Waiting for initial button release.\n");
        PBIO_OS_AWAIT_WHILE(state, ({
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                return PBIO_ERROR_CANCELED;
            }
            pbdrv_button_get_pressed();
        }));

        DEBUG_PRINT("Start waiting for input.\n");
        // Wait on a button, external program start, or connection change. Stop
        // waiting on timeout or shutdown.
        PBIO_OS_AWAIT_UNTIL(state, ({
            // Shutdown may be requested by a background process such as critical
            // battery or holding the power button.
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                return PBIO_ERROR_CANCELED;
            }

            // Exit on timeout except while connected to host.
            if (pbsys_host_is_connected()) {
                pbio_os_timer_set(timer, timer->duration);
            } else if (pbio_os_timer_is_expired(timer)) {
                return PBIO_ERROR_TIMEDOUT;
            }

            // Wait for button press, external program start, or connection change.
            pbdrv_button_get_pressed() ||
            pbsys_main_program_start_is_requested() ||
            pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE) != previously_connected;
        }));

        // Became connected or disconnected, so go back to handle it.
        if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE) != previously_connected) {
            DEBUG_PRINT("Connection changed.\n");
            continue;
        }

        // External progran request takes precedence over buttons.
        if (pbsys_main_program_start_is_requested()) {
            DEBUG_PRINT("Start program from Pybricks Code.\n");
            break;
        }

        #if PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON
        // Toggle Bluetooth enable setting if Bluetooth button pressed. Only if disconnected.
        if ((pbdrv_button_get_pressed() & PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON) && !pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)) {
            pbsys_storage_settings_bluetooth_enabled_set(!pbsys_storage_settings_bluetooth_enabled_get());
            DEBUG_PRINT("Toggling Bluetooth to: %s. \n", pbsys_storage_settings_bluetooth_enabled_get() ? "on" : "off");
            continue;
        }
        #endif // PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON

        #if PBSYS_CONFIG_HMI_PUP_LEFT_RIGHT_BUTTONS
        // On right, increment slot when possible, then start waiting on new inputs.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_RIGHT) {
            pbsys_status_increment_selected_slot(true);
            continue;
        }
        // On left, decrement slot when possible, then start waiting on new inputs.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_LEFT) {
            pbsys_status_increment_selected_slot(false);
            continue;
        }
        #endif // PBSYS_CONFIG_HMI_PUP_LEFT_RIGHT_BUTTONS

        // On center, attempt to start program.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_CENTER) {
            pbio_error_t err = pbsys_main_program_request_start(pbsys_status_get_selected_slot(), PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);
            if (err == PBIO_SUCCESS) {
                DEBUG_PRINT("Start program with button\n");
                break;
            } else {
                DEBUG_PRINT("Requested program not available.\n");
                // We can run an animation here to indicate that the program is not available.
            }
        }

        DEBUG_PRINT("No valid action selected, start over.\n");
    }

    // Stop advertising if we are still doing so.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING)) {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
        DEBUG_PRINT("Stop advertising on HMI exit.\n");
        pbdrv_bluetooth_start_advertising(false);
        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_await_advertise_or_scan_command(&sub, NULL));
    }

    // Wait for all buttons to be released so the user doesn't accidentally
    // push their robot off course.
    DEBUG_PRINT("Waiting for final button release.\n");
    PBIO_OS_AWAIT_WHILE(state, ({
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }
        pbdrv_button_get_pressed();
    }));
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

    pbio_os_timer_t idle_timer;
    pbio_os_timer_set(&idle_timer, PBSYS_CONFIG_HMI_IDLE_TIMEOUT_MS);

    pbio_os_state_t state = 0;

    pbio_error_t err;
    while ((err = run_ui(&state, &idle_timer)) == PBIO_ERROR_AGAIN) {
        // run all processes and wait for next event.
        pbio_os_run_processes_and_wait_for_event();
    }
    DEBUG_PRINT("Finished program selection with status: %d\n", err);
    return err;
}

#endif // PBSYS_CONFIG_HMI_PUP

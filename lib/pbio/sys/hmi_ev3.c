// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user for systems
// with directional buttons and an EV3 display.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_EV3

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/display.h>
#include <pbdrv/usb.h>

#include <pbio/button.h>
#include <pbio/os.h>
#include <pbsys/host.h>
#include <pbio/image.h>
#include <pbsys/light.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/storage.h>
#include <pbsys/storage_settings.h>

#include "hmi.h"
#include "storage.h"
#include "hmi_ev3_ui.h"


#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

static void pbsys_hmi_host_update_indications(void) {
    if (pbdrv_usb_connection_is_active()) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);
    } else {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);
    }
}

static bool pbsys_hmi_handle_connection_change;

/**
 * Called from the USB and Bluetooth driver if a host connection state changes.
 */
static void pbsys_hmi_connection_changed_callback(void) {
    DEBUG_PRINT("A host connected or disconnected.\n");
    pbsys_hmi_handle_connection_change = true;
    pbsys_hmi_host_update_indications();
}


void pbsys_hmi_init(void) {
    pbdrv_usb_set_host_connection_changed_callback(pbsys_hmi_connection_changed_callback);
}

void pbsys_hmi_deinit(void) {

    pbdrv_usb_set_host_connection_changed_callback(NULL);
    pbsys_status_clear(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);

    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, 0);
    pbdrv_display_update();
}

static pbio_error_t run_ui(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    PBIO_OS_ASYNC_BEGIN(state);

    pbsys_hmi_ev3_ui_initialize();

    for (;;) {

        DEBUG_PRINT("Start HMI loop\n");

        pbsys_hmi_host_update_indications();

        pbsys_hmi_ev3_ui_draw();

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
                pbio_os_timer_reset(timer);
            } else if (pbio_os_timer_is_expired(timer)) {
                return PBIO_ERROR_TIMEDOUT;
            }

            // Wait for button press, external program start, or connection change.
            pbdrv_button_get_pressed() || pbsys_main_program_start_is_requested() || pbsys_hmi_handle_connection_change;
        }));

        // On setting or closing a connection, start from a clean slate.
        if (pbsys_hmi_handle_connection_change) {
            pbsys_hmi_handle_connection_change = false;
            continue;
        }

        // External progran request takes precedence over buttons.
        if (pbsys_main_program_start_is_requested()) {
            DEBUG_PRINT("Start program from Pybricks Code.\n");
            break;
        }

        // Update UI state for buttons.
        uint8_t payload;
        pbsys_hmi_ev3_ui_action_t action = pbsys_hmi_ev3_ui_handle_button(pbdrv_button_get_pressed(), &payload);

        if (action == PBSYS_HMI_EV3_UI_ACTION_SET_SLOT) {
            pbsys_status_set_selected_slot(payload);
            continue;
        }

        if (action == PBSYS_HMI_EV3_UI_ACTION_SHUTDOWN) {
            return PBIO_ERROR_CANCELED;
        }

        if (action == PBSYS_HMI_EV3_UI_ACTION_RUN_PROGRAM) {
            pbio_error_t err = pbsys_main_program_request_start(payload, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);
            if (err != PBIO_SUCCESS) {
                DEBUG_PRINT("Requested program not available.\n");
                pbsys_hmi_ev3_ui_handle_error(err);
                continue;
            }

            // Exit HMI loop if valid program selected.
            DEBUG_PRINT("Start program with button\n");
            break;
        }
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

    // Start light or display animations.
    #if PBIO_CONFIG_LIGHT
    pbio_color_light_start_breathe_animation(pbsys_status_light_main, PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS_HUE);
    #endif

    pbsys_hmi_ev3_ui_run_animation_start();

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

#endif // PBSYS_CONFIG_HMI_EV3

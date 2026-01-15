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
#include <pbdrv/led.h>
#include <pbdrv/usb.h>

#include <pbio/button.h>
#include <pbio/busy_count.h>
#include <pbio/light_matrix.h>
#include <pbio/os.h>
#include <pbsys/host.h>
#include <pbsys/light.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/storage_settings.h>

#include "hmi.h"

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

#if PBIO_CONFIG_LIGHT_MATRIX

pbio_light_matrix_t *pbsys_hub_light_matrix;

/**
 * Displays the idle UI. Has a square stop sign and selected slot on bottom row.
 *
 * @param brightness   Brightness (0--100%).
 */
static void light_matrix_show_idle_ui(uint8_t brightness) {
    for (uint8_t r = 0; r < PBSYS_CONFIG_HMI_NUM_SLOTS; r++) {
        for (uint8_t c = 0; c < PBSYS_CONFIG_HMI_NUM_SLOTS; c++) {
            bool is_on = r < 3 && c > 0 && c < 4;
            is_on |= (r == 4 && c == pbsys_status_get_selected_slot());
            pbio_light_matrix_set_pixel(pbsys_hub_light_matrix, r, c, is_on ? brightness : 0, true);
        }
    }
}

/**
 * Bootup and shutdown animation. This is a process rather than an animation
 * process so we can await completion, and later add sound.
 */
static pbio_error_t boot_animation_process_boot_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;
    static uint8_t step;

    const int num_steps = 10;

    // Makes the brightness increment or decrement.
    bool booting = context;

    PBIO_OS_ASYNC_BEGIN(state);

    for (step = 1; step <= num_steps; step++) {
        uint8_t brightness = booting ? step * (100 / num_steps) : 100 - (100 / num_steps) * step;
        light_matrix_show_idle_ui(brightness);
        PBIO_OS_AWAIT_MS(state, &timer, 200 / num_steps);
    }
    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// Boot and shutdown animation are the same.
static pbio_os_process_func_t boot_animation_process_shutdown_thread = boot_animation_process_boot_thread;

/**
 * Animation frame for program running animation.
 */
static uint32_t pbio_light_matrix_5x5_spinner_animation_next(pbio_light_animation_t *animation) {

    // The indexes of pixels to light up
    static const uint8_t indexes[] = { 1, 2, 3, 8, 13, 12, 11, 6 };

    // Each pixel has a repeating brightness pattern of the form /\_ through
    // which we can cycle in 256 steps.
    static uint8_t cycle = 0;

    for (size_t i = 0; i < PBIO_ARRAY_SIZE(indexes); i++) {
        // The pixels are spread equally across the pattern.
        uint8_t offset = cycle + i * (UINT8_MAX / PBIO_ARRAY_SIZE(indexes));
        uint8_t brightness = offset > 200 ? 0 : (offset < 100 ? offset : 200 - offset);

        // Set the brightness for this pixel
        pbio_light_matrix_set_pixel(pbsys_hub_light_matrix, indexes[i] / 5, indexes[i] % 5, brightness, false);
    }
    // This increment controls the speed of the pattern
    cycle += 9;

    return 40;
}

static void light_matrix_start_run_animation(void) {
    // Central pixel in spinner is off and will not be updated.
    pbio_light_matrix_set_pixel(pbsys_hub_light_matrix, 1, 2, 0, true);
    pbio_light_animation_init(&pbsys_hub_light_matrix->animation, pbio_light_matrix_5x5_spinner_animation_next);
    pbio_light_animation_start(&pbsys_hub_light_matrix->animation);
}

#else

/**
 * This shutdown "animation" just pauses for half a second. The actual animation
 * is handled by the system light based on the shutdown status.
 */
static pbio_error_t boot_animation_process_shutdown_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT_MS(state, &timer, 500);
    pbio_busy_count_down();
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#endif

static void pbsys_hmi_host_update_indications(void) {
    // Update USB light indication.
    if (pbdrv_usb_connection_is_active()) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);
    } else {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);
    }

    // Update BLE light indication.
    if (pbdrv_bluetooth_host_is_connected()) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
    } else {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
    }
}

static bool pbsys_hmi_handle_connection_change;

/**
 * Called from the USB and Bluetooth driver if a host connection changes.
 *
 * This pertains to the actual Pybricks profile/events (un)subscribe, not
 * necessarily the physical link.
 */
static void pbsys_hmi_connection_changed_callback(void) {
    DEBUG_PRINT("A host connected or disconnected.\n");
    pbsys_hmi_handle_connection_change = true;
    pbsys_hmi_host_update_indications();
}

static pbio_os_process_t boot_animation_process;

void pbsys_hmi_init(void) {

    pbdrv_usb_set_host_connection_changed_callback(pbsys_hmi_connection_changed_callback);
    pbdrv_bluetooth_set_host_connection_changed_callback(pbsys_hmi_connection_changed_callback);

    #if PBIO_CONFIG_LIGHT_MATRIX
    pbio_busy_count_up();
    pbio_error_t err = pbio_light_matrix_get_dev(0, 5, &pbsys_hub_light_matrix);
    if (err != PBIO_SUCCESS) {
        // Effectively stopping boot if we can't get hardware.
        return;
    }
    pbio_os_process_start(&boot_animation_process, boot_animation_process_boot_thread, (void *)true);
    #endif
}

void pbsys_hmi_deinit(void) {

    pbdrv_usb_set_host_connection_changed_callback(NULL);
    pbdrv_bluetooth_set_host_connection_changed_callback(NULL);

    pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
    pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
    pbsys_status_clear(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);

    pbio_busy_count_up();
    pbio_os_process_start(&boot_animation_process, boot_animation_process_shutdown_thread, (void *)false);
}

/**
 * Convenience wrapper to start or stop advertising, set status, and await command.
 *
 * The system is allowed to be in the requested state already. Then this is a no-op.
 *
 * @param [in]  state       The protothread state.
 * @param [in]  advertise   true to start advertising, false to stop.
 */
static pbio_error_t start_advertising(pbio_os_state_t *state, bool advertise) {

    pbio_error_t err;
    pbio_os_state_t unused;

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_bluetooth_start_advertising(advertise);
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_advertise_or_scan_command(&unused, NULL));

    if (advertise && err == PBIO_SUCCESS) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
    } else {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
    }

    DEBUG_PRINT("BLE advertising is: %d (requested %d) with error %d. \n", pbsys_status_test(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING), advertise, err);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * The HMI is a loop running the following steps:
 *
 *    - Wait for any buttons to be released in case they were pressed
 *    - Wait for a button press, external program start, while monitoring idle
 *      timeout and BLE advertising.
 *    - If valid program requested, break out of loop to start program.
 *      Otherwise, update state based on what happened and start over.
 *    - After leaving the loop, wait for all buttons to be released.
 *
 * The three waiting operations are cancelled if poweroff is requested.
 */
static pbio_error_t run_ui(pbio_os_state_t *state) {

    static pbio_os_state_t sub;

    static pbio_os_timer_t idle_timer;

    // Used as a local variable in a few places, but the value needs to persist
    // during the async start advertising call.
    static bool should_advertise;

    PBIO_OS_ASYNC_BEGIN(state);

    pbio_os_timer_set(&idle_timer, PBSYS_CONFIG_HMI_IDLE_TIMEOUT_MS);

    // Always start by setting initial connection state indications.
    pbsys_hmi_handle_connection_change = true;

    for (;;) {

        DEBUG_PRINT("Start HMI loop\n");

        // Visually indicate current state on supported hubs.
        #if PBIO_CONFIG_LIGHT_MATRIX
        light_matrix_show_idle_ui(100);
        #endif

        pbsys_hmi_host_update_indications();

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
                pbio_os_timer_reset(&idle_timer);
            } else if (pbio_os_timer_is_expired(&idle_timer)) {
                return PBIO_ERROR_TIMEDOUT;
            }

            // Wait condition: button pressed, program start requested, or connection change.
            pbdrv_button_get_pressed() || pbsys_main_program_start_is_requested() || pbsys_hmi_handle_connection_change;
        }));

        // On setting or closing a connection, start from a clean slate:
        // Begin advertising if Bluetooth enabled and there is no host
        // connection, otherwise disable.
        if (pbsys_hmi_handle_connection_change) {
            pbsys_hmi_handle_connection_change = false;
            should_advertise = pbsys_storage_settings_bluetooth_enabled_get() && !pbsys_host_is_connected();
            PBIO_OS_AWAIT(state, &sub, start_advertising(&sub, should_advertise));
            continue;
        }

        // External program request takes precedence over buttons.
        if (pbsys_main_program_start_is_requested()) {
            DEBUG_PRINT("Start program from Pybricks Code.\n");
            break;
        }

        #if PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON
        // Handle Bluetooth button press.
        if (pbdrv_button_get_pressed() & PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON) {

            should_advertise = false;

            // Button behavior depends on whether we're already connected.
            if (pbsys_host_is_connected()) {

                // If already connected, pressing toggles advertising to allow
                // for a secondary host connection.
                should_advertise = !pbsys_status_test(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);

                // We could be connected to USB and BLE might have been
                // configured off previously. So turn back on.
                if (!pbsys_storage_settings_bluetooth_enabled_get()) {
                    pbsys_storage_settings_bluetooth_enabled_set(true);
                }
            } else {
                // When disconnected, pressing the button toggles Bluetooth enable
                // state, and Bluetooth advertising is updated to match.
                pbsys_storage_settings_bluetooth_enabled_set(!pbsys_storage_settings_bluetooth_enabled_get());
                should_advertise = pbsys_storage_settings_bluetooth_enabled_get();
            }

            // Set requested state.
            PBIO_OS_AWAIT(state, &sub, start_advertising(&sub, should_advertise));

            // Go back to wait for button release and other input.
            continue;
        }
        #endif // PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON

        #if PBSYS_CONFIG_HMI_PUP_LEFT_RIGHT_ENABLE
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
        #endif // PBSYS_CONFIG_HMI_PUP_LEFT_RIGHT_ENABLE

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

    // Wait for all buttons to be released so the user doesn't accidentally
    // push their robot off course.
    DEBUG_PRINT("Waiting for final button release.\n");
    PBIO_OS_AWAIT_WHILE(state, ({
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }
        pbdrv_button_get_pressed();
    }));

    // Stop advertising if we are still doing so.
    DEBUG_PRINT("Stop advertising on HMI exit.\n");
    PBIO_OS_AWAIT(state, &sub, start_advertising(&sub, false));

    // Start run animations
    #if PBIO_CONFIG_LIGHT_MATRIX
    light_matrix_start_run_animation();
    #endif

    #if PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS
    pbio_color_light_start_breathe_animation(pbsys_status_light_main, PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS_HUE);
    #elif PBSYS_CONFIG_STATUS_LIGHT
    pbio_color_light_off(pbsys_status_light_main);
    #endif

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

    pbio_os_state_t state = 0;

    pbio_error_t err;
    while ((err = run_ui(&state)) == PBIO_ERROR_AGAIN) {
        // run all processes and wait for next event.
        pbio_os_run_processes_and_wait_for_event();
    }
    DEBUG_PRINT("Finished program selection with status: %d\n", err);
    return err;
}

#endif // PBSYS_CONFIG_HMI_PUP

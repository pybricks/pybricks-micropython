// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_MAIN

#include <stdint.h>

#include <pbdrv/core.h>
#include <pbdrv/reset.h>
#include <pbdrv/usb.h>
#include <pbio/main.h>
#include <pbio/os.h>
#include <pbio/port_interface.h>
#include <pbio/protocol.h>
#include <pbsys/core.h>
#include <pbsys/main.h>
#include <pbsys/status.h>

#include "hmi.h"
#include "program_stop.h"
#include "storage.h"
#include <pbsys/program_stop.h>
#include <pbsys/light.h>
#include <pbsys/host.h>

// Singleton with information about the currently (or soon) active program.
static pbsys_main_program_t program;

bool pbsys_main_program_start_is_requested() {
    return program.start_request_type != PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_NONE;
}

/**
 * Gets the type of start request for the main program.
 *
 * @param [in]  program A pointer to the main program structure.
 * @returns     The type of start request.
 */
pbsys_main_program_start_request_type_t pbsys_main_program_get_start_request_type(void) {
    return program.start_request_type;
}

/**
 * Requests to start the main user application program.
 *
 * @param [in]  type    Chooses to start a builtin user program or a user program.
 * @param [in]  id      Selects which builtin or user program will run.
 * @returns     ::PBIO_ERROR_BUSY if a user program is already running.
 *              ::PBIO_ERROR_NOT_SUPPORTED if the program is not available.
 *              Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_main_program_request_start(pbio_pybricks_user_program_id_t id, pbsys_main_program_start_request_type_t start_request_type) {

    // Can't start new program if already running or new requested.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING) || pbsys_main_program_start_is_requested()) {
        return PBIO_ERROR_BUSY;
    }

    program.id = id;

    // Load applicable data for this slot.
    pbsys_storage_get_program_data(&program);

    pbio_error_t err = pbsys_main_program_validate(&program);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    program.start_request_type = start_request_type;

    return PBIO_SUCCESS;
}

/**
 * Initializes the PBIO library, runs custom main program, and handles shutdown.
 */
void pbsys_main(void) {

    pbdrv_init();
    pbio_init();
    pbsys_init();

    // Keep loading and running user programs until shutdown is requested.
    for (;;) {

        // Drives all processes while waiting for user input. This completes
        // when a user program request is made using the buttons or by a
        // connected host. It is cancelled on shutdown request or idle timeout.
        pbio_error_t err = pbsys_hmi_await_program_selection();
        if (err != PBIO_SUCCESS) {
            // Shutdown requested or idle for a long time.
            break;
        }

        // Prepare pbsys for running the program.
        pbsys_status_set_program_id(program.id);
        pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
        pbsys_host_stdin_set_callback(pbsys_main_stdin_event);

        // Handle pending events triggered by the status change, such as
        // starting status light animation.
        while (pbio_os_run_processes_once()) {
        }

        // Make sure we are starting with an empty stdin buffer.
        pbsys_host_stdin_flush();

        // Run the main application.
        pbsys_main_run_program(&program);

        // Stop motors, user animations, user bluetooth activity, etc.
        err = pbio_main_stop_application_resources();
        if (err != PBIO_SUCCESS) {
            // If we couldn't get the system back in a normal state, proceed
            // towards shutdown.
            break;
        }

        // Get system back in idle state.
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
        pbsys_host_stdin_set_callback(NULL);
        pbsys_program_stop_set_buttons(PBSYS_CONFIG_HMI_STOP_BUTTON);
        program.start_request_type = PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_NONE;

        // Handle pending events triggered by the status change, such as
        // stopping status light animation.
        while (pbio_os_run_processes_once()) {
        }

        // Finalize application now that system resources are safely closed.
        pbsys_main_run_program_cleanup();
    }

    // Stop system processes and selected drivers in reverse order. This will
    // also save user data to flash,
    pbsys_deinit();
    pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN);

    // Now lower-level processes may shutdown and/or power off.
    pbio_deinit();
    pbdrv_deinit();

    // The power could be held on due to someone pressing the center button
    // so we have this loop to keep handling events to drive processes that
    // turn off some of the peripherals.
    while (pbsys_status_test(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED)) {
        pbio_os_run_processes_and_wait_for_event();
    }

    #if PBSYS_CONFIG_BATTERY_CHARGER
    // Similarly, run events to keep charging while the hub is "off".
    while (pbdrv_usb_get_bcd() != PBDRV_USB_BCD_NONE) {
        pbio_os_run_processes_and_wait_for_event();
        // If the button is pressed again, the user wants to turn the hub
        // "back on". We are still on, so do a full reset instead.
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED)) {
            pbdrv_reset(PBDRV_RESET_ACTION_RESET);
        }
    }
    #endif

    // Platform-specific power off.
    pbdrv_reset_power_off();
}

#endif // PBSYS_CONFIG_MAIN

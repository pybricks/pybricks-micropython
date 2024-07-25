// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_MAIN

#include <stdint.h>

#include <pbdrv/reset.h>
#include <pbdrv/usb.h>
#include <pbio/main.h>
#include <pbio/protocol.h>
#include <pbsys/core.h>
#include <pbsys/main.h>
#include <pbsys/status.h>

#include "program_stop.h"
#include "storage.h"
#include <pbsys/program_stop.h>
#include <pbsys/bluetooth.h>

static pbsys_main_program_t program;

#include <stdio.h>

/**
 * Requests to start the main user application program.
 *
 * @param [in]  type    Chooses to start a builtin program or a user program.
 * @param [in]  id      Selects which builtin or user program will run.
 * @returns     ::PBIO_ERROR_BUSY if a user program is already running.
 *              ::PBIO_ERROR_NOT_SUPPORTED if the program is not available.
 *              Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_main_program_request_start(pbsys_main_program_type_t type, uint32_t id) {

    // Can't start new program if already running or new requested.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING) || program.start_requested) {
        return PBIO_ERROR_BUSY;
    }

    program.type = type;
    program.id = id;

    // Builtin programs are also allowed to access user program,
    // so load data in all cases.
    pbsys_storage_get_program_data(&program);

    pbio_error_t err = pbsys_main_program_validate(&program);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    program.start_requested = true;

    return PBIO_SUCCESS;
}

/**
 * Initializes the PBIO library, runs custom main program, and handles shutdown.
 *
 * @param [in]  main    The main program.
 */
int main(int argc, char **argv) {

    pbio_init();
    pbsys_init();

    // Keep loading and running user programs until shutdown is requested.
    while (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {

        // REVISIT: this can be long waiting, so we could do a more efficient
        // wait (i.e. __WFI() on embedded system)
        while (pbio_do_one_event()) {
        }

        if (!program.start_requested) {
            continue;
        }

        // Prepare pbsys for running the program.
        pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
        pbsys_bluetooth_rx_set_callback(pbsys_main_stdin_event);

        // Handle pending events triggered by the status change, such as
        // starting status light animation.
        while (pbio_do_one_event()) {
        }

        // Run the main application.
        pbsys_main_run_program(&program);

        // Get system back in idle state.
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
        pbsys_bluetooth_rx_set_callback(NULL);
        pbsys_program_stop_set_buttons(PBIO_BUTTON_CENTER);
        pbio_stop_all(true);
        program.start_requested = false;
    }

    // Stop system processes and save user data before we shutdown.
    pbsys_deinit();

    // Now lower-level processes may shutdown and/or power off.
    pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN);

    // The power could be held on due to someone pressing the center button
    // or USB being plugged in, so we have this loop to keep pumping events
    // to turn off most of the peripherals and keep the battery charger running.
    for (;;) {
        // We must handle all pending events before turning the power off the
        // first time, otherwise the city hub turns itself back on sometimes.
        while (pbio_do_one_event()) {
        }

        #if PBSYS_CONFIG_BATTERY_CHARGER
        // On hubs with USB battery chargers, we can't turn off power while
        // USB is connected, otherwise it disables the op-amp that provides
        // the battery voltage to the ADC.
        if (pbdrv_usb_get_bcd() != PBDRV_USB_BCD_NONE) {
            continue;
        }
        #endif

        pbdrv_reset_power_off();
    }
}

#endif // PBSYS_CONFIG_MAIN

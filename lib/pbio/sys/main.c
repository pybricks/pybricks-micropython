// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdint.h>

#include <contiki.h>

#include <pbdrv/clock.h>
#include <pbdrv/reset.h>

#include <pbio/main.h>

#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <pbsys/system.h>
#include <pbsys/user_program.h>

#include "user_program.h"

/**
 * Initializes the PBIO library, runs custom main program, and handles shutdown.
 *
 */
int main(int argc, char **argv) {
    pbio_init();
    pbsys_init();

    while (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        // Load program info and start process to wait for program.
        pbsys_user_program_info_t *info;
        pbsys_user_program_process_start(&info);

        // Wait for program receive process to complete.
        while (!pbsys_user_program_process_complete()) {
            pbio_do_one_event();
        }

        // Run the main application.
        pbsys_user_program_application_main(info);
    }

    // The power could be held on due to someone pressing the center button
    // or USB being plugged in, so we have this loop to keep pumping events
    // to turn off most of the peripherals and keep the battery charger running.
    for (;;) {
        // We must handle all pending events before turning the power off the
        // first time, otherwise the city hub turns itself back on sometimes.
        while (pbio_do_one_event()) {
        }
        pbdrv_reset_power_off();
    }
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_MAIN

#include <stdint.h>

#include <pbdrv/reset.h>
#include <pbdrv/usb.h>
#include <pbio/main.h>
#include <pbsys/core.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>

#include "program_load.h"

/**
 * Initializes the PBIO library, runs custom main program, and handles shutdown.
 *
 * @param [in]  main    The main program.
 */
int main(int argc, char **argv) {

    pbio_init();
    pbsys_init();

    // Keep loading and running user programs until shutdown flag is set.
    while (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {

        // Receive a program. This cancels itself on shutdown.
        static pbsys_main_program_t program;
        pbio_error_t err = pbsys_program_load_receive(&program);
        if (err != PBIO_SUCCESS) {
            continue;
        }

        // Run the main application. The application must register appropriate
        // callbacks in sys/user_program so it can be canceled on shutdown.
        pbsys_main_application(&program);
    }

    // Stop system processes and save user data before we shutdown.
    pbsys_deinit();

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

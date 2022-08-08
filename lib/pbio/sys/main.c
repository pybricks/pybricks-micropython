// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdint.h>

#include <pbdrv/clock.h>
#include <pbdrv/reset.h>

#include <pbio/main.h>

#include <pbsys/bluetooth.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>
#include "user_program.h"

static void *pbsys_main_jmp_buf[5];

static void pb_sys_main_check_for_shutdown(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        pbio_set_event_hook(NULL);
        __builtin_longjmp(pbsys_main_jmp_buf, 1);
    }
}

/**
 * Initializes the PBIO library and runs custom main program.
 *
 * The main program may be abruptly ended when shutting down the hub.
 *
 * @param [in]  main    The main program.
 */
void pbsys_main(pbsys_main_t main) {
    pbio_init();

    // REVISIT: __builtin_setjmp() only saves a couple registers, so using it
    // could cause problems if we add more to this function. However, since we
    // use -nostdlib compile flag, we don't have setjmp(). We should be safe
    // for now though since we don't use any local variables after the longjmp.
    if (__builtin_setjmp(pbsys_main_jmp_buf) == 0) {
        // REVISIT: we could save a few CPU cycles on each call to pbio_do_one_event()
        // if we don't set this until shutdown is actually requested
        pbio_set_event_hook(pb_sys_main_check_for_shutdown);
        for (;;) {
            // Load program info and start process to wait for program.
            pbsys_user_program_info_t *info;
            pbsys_user_program_process_start(&info);

            // Wait for program process to complete.
            while (!pbsys_user_program_process_complete()) {
                pbio_do_one_event();
            }

            // Run the main application.
            main(info);
        }
    } else {
        // in case we jumped out of the middle of a user program
        pbsys_user_program_unprepare();
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

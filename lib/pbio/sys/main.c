// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdint.h>

#include <contiki.h>

#include <pbdrv/clock.h>
#include <pbdrv/reset.h>

#include <pbio/main.h>

#include <pbsys/battery.h>
#include <pbsys/bluetooth.h>
#include <pbsys/program_load.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>

#include "hmi.h"
#include "io_ports.h"
#include "main.h"
#include "program_load.h"
#include "supervisor.h"
#include "user_program.h"

PROCESS(pbsys_system_process, "System");

PROCESS_THREAD(pbsys_system_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    etimer_set(&timer, 50);

    while (true) {
        PROCESS_WAIT_EVENT();
        pbsys_hmi_handle_event(ev, data);
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);
            pbsys_battery_poll();
            pbsys_hmi_poll();
            pbsys_io_ports_poll();
            pbsys_supervisor_poll();
            pbsys_user_program_poll();
        }
    }

    PROCESS_END();
}

void pbsys_init() {
    pbsys_battery_init();
    pbsys_bluetooth_init();
    pbsys_hmi_init();
    process_start(&pbsys_system_process);
}

#if PBSYS_CONFIG_PROGRAM_LOAD

/**
 * Initializes the PBIO library, runs custom main program, and handles shutdown.
 *
 */
int main(int argc, char **argv) {
    pbio_init();
    pbsys_init();

    while (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        // Load program info and start process to wait for program.
        pbsys_program_load_receive_start();

        // Wait for program receive process to complete.
        while (!pbsys_program_load_receive_complete()) {
            pbio_do_one_event();
        }

        // Run the main application.
        pbsys_program_load_info_t *info = pbsys_program_load_get_info();
        pbsys_program_load_application_main(info);
    }

    pbio_deinit();

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

#endif // PBSYS_CONFIG_PROGRAM_LOAD

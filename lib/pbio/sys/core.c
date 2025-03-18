// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdint.h>

#include <pbio/main.h>
#include <pbio/os.h>

#include <pbsys/battery.h>
#include <pbsys/host.h>
#include <pbsys/status.h>

#include "core.h"
#include "hmi.h"
#include "light.h"
#include "storage.h"
#include "supervisor.h"
#include "program_stop.h"

uint32_t pbsys_init_busy_count;

PROCESS(pbsys_system_process, "System");

PROCESS_THREAD(pbsys_system_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    etimer_set(&timer, 50);

    for (;;) {
        PROCESS_WAIT_EVENT();

        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);
            pbsys_battery_poll();
            pbsys_hmi_poll();
            pbsys_supervisor_poll();
            pbsys_program_stop_poll();
        }
    }

    PROCESS_END();
}

void pbsys_init(void) {
    pbsys_battery_init();
    pbsys_host_init();
    pbsys_hmi_init();
    pbsys_storage_init();
    process_start(&pbsys_system_process);

    while (pbsys_init_busy()) {
        pbio_os_run_processes_once();
    }
}

void pbsys_deinit(void) {

    pbsys_storage_deinit();

    uint32_t start = pbdrv_clock_get_ms();

    // Wait for all relevant pbsys processes to end, but at least 500 ms so we
    // see a shutdown animation even if the button is released sooner.
    while (pbsys_init_busy() || pbdrv_clock_get_ms() - start < 500) {
        pbio_os_run_processes_once();
    }
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdint.h>

#include <contiki.h>

#include <pbio/main.h>

#include <pbsys/battery.h>
#include <pbsys/bluetooth.h>

#include "core.h"
#include "hmi.h"
#include "io_ports.h"
#include "program_load.h"
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
        pbsys_hmi_handle_event(ev, data);
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);
            pbsys_battery_poll();
            pbsys_hmi_poll();
            pbsys_io_ports_poll();
            pbsys_supervisor_poll();
            pbsys_program_stop_poll();
        }
    }

    PROCESS_END();
}

void pbsys_init(void) {
    pbsys_battery_init();
    pbsys_bluetooth_init();
    pbsys_hmi_init();
    pbsys_program_load_init();
    process_start(&pbsys_system_process);

    while (pbsys_init_busy()) {
        pbio_process_events();
    }
}

void pbsys_deinit(void) {
    pbsys_program_load_deinit();

    uint32_t start = pbdrv_clock_get_ms();

    // Wait for all relevant pbsys processes to end, but at least 500 ms so we
    // see a shutdown animation even if the button is released sooner.
    while (pbsys_init_busy() || pbdrv_clock_get_ms() - start < 500) {
        pbio_process_events();
    }
}

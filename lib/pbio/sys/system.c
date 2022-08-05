// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdint.h>

#include <contiki.h>

#include <pbsys/battery.h>
#include <pbsys/bluetooth.h>

#include "hmi.h"
#include "io_ports.h"
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

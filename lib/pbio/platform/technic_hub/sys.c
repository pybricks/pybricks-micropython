// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

#include <contiki.h>

#include "pbio/event.h"
#include "pbio/main.h"

#include <pbsys/battery.h>
#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <pbsys/supervisor.h>
#include <pbsys/user_program.h>

#include "../sys/hmi.h"

PROCESS(pbsys_process, "System");


PROCESS_THREAD(pbsys_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    pbsys_battery_init();
    pbsys_bluetooth_init();
    pbsys_hmi_init();
    etimer_set(&timer, clock_from_msec(50));

    while (true) {
        PROCESS_WAIT_EVENT();
        pbsys_hmi_handle_event(ev, data);
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);
            pbsys_battery_poll();
            pbsys_hmi_poll();
            pbsys_supervisor_poll();
        } else if (ev == PBIO_EVENT_STATUS_SET) {
            if ((pbio_pybricks_status_t)data == PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED) {
                pbsys_user_program_stop();
            }
        }
    }

    PROCESS_END();
}

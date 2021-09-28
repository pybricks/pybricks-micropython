// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <contiki.h>

#include "pbio/event.h"

#include <pbsys/status.h>

#include "../sys/hmi.h"
#include "../sys/user_program.h"

PROCESS(pbsys_process, "System");

PROCESS_THREAD(pbsys_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    pbsys_hmi_init();
    etimer_set(&timer, 50);

    while (true) {
        PROCESS_WAIT_EVENT();
        pbsys_hmi_handle_event(ev, data);
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);
            pbsys_hmi_poll();
            pbsys_user_program_poll();
        }
    }

    PROCESS_END();
}

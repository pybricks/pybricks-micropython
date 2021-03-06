// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <contiki.h>

#include "pbio/event.h"
#include "pbio/main.h"

#include <pbsys/battery.h>
#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <pbsys/supervisor.h>
#include <pbsys/sys.h>

#include "../sys/hmi.h"

// user program stop function
static pbsys_stop_callback_t user_stop_func;

PROCESS(pbsys_process, "System");

void pbsys_prepare_user_program(const pbsys_user_program_callbacks_t *callbacks) {
    if (callbacks) {
        user_stop_func = callbacks->stop;
        pbsys_bluetooth_rx_set_callback(callbacks->stdin_event);
    } else {
        user_stop_func = NULL;
        pbsys_bluetooth_rx_set_callback(NULL);
    }
    pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
}

void pbsys_unprepare_user_program(void) {
    pbsys_status_clear(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
    pbio_stop_all();
    user_stop_func = NULL;
    pbsys_bluetooth_rx_set_callback(NULL);
}

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
            if ((pbio_pybricks_status_t)data == PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED && user_stop_func) {
                user_stop_func();
            }
        } else if (ev == PBIO_EVENT_COM_CMD) {
            pbio_com_cmd_t cmd = (uint32_t)data;

            switch (cmd) {
                case PBIO_COM_CMD_START_USER_PROGRAM:
                    break;
                case PBIO_COM_CMD_STOP_USER_PROGRAM:
                    if (user_stop_func) {
                        user_stop_func();
                    }
                    break;
            }
        }
    }

    PROCESS_END();
}

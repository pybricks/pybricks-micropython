// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdint.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>
#include <pbdrv/watchdog.h>
#include <pbdrv/usb.h>

#include <pbio/busy_count.h>
#include <pbio/main.h>
#include <pbio/os.h>

#include <pbsys/battery.h>
#include <pbsys/host.h>
#include <pbsys/status.h>

#include "hmi.h"
#include "light.h"
#include "storage.h"
#include "program_stop.h"
#include "telemetry.h"

static pbio_os_process_t pbsys_system_poll_process;

static pbio_error_t pbsys_system_poll_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    pbio_os_timer_set(&timer, 50);

    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&timer));
        pbio_os_timer_extend(&timer);

        pbsys_battery_poll();
        pbsys_program_stop_poll();
        pbsys_status_light_poll();
        pbsys_storage_poll();

        // keep the hub from resetting itself
        pbdrv_watchdog_update();
    }

    // Unreachable
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

void pbsys_init(void) {

    // Makes user data and settings available to modules below, so must be done first.
    pbsys_storage_init();

    pbsys_battery_init();
    pbsys_hmi_init();
    pbsys_host_init();
    pbsys_status_light_init();
    pbsys_telemetry_init();

    pbio_os_process_start(&pbsys_system_poll_process, pbsys_system_poll_process_thread, NULL);

    while (pbio_busy_count_busy()) {
        pbio_os_run_processes_and_wait_for_event();
    }
}

void pbsys_deinit(void) {

    // Used by status indications during shutdown.
    pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);

    pbsys_storage_deinit();
    pbsys_hmi_deinit();

    // Wait for all relevant pbsys processes to end.
    while (pbio_busy_count_busy()) {
        pbio_os_run_processes_and_wait_for_event();
    }
}

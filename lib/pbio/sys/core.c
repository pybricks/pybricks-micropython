// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdint.h>

#include <pbdrv/bluetooth.h>
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

        // keep the hub from resetting itself
        pbdrv_watchdog_update();

        if (pbsys_system_poll_process.request == PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL) {
            // After shutdown we only poll critical system processes.
            continue;
        }

        // Monitor USB state.
        if (pbdrv_usb_connection_is_active()) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);
        } else {
            pbsys_status_clear(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED);
        }

        // Monitor BLE state.
        if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
        } else {
            pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED);
        }
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

    pbio_os_process_start(&pbsys_system_poll_process, pbsys_system_poll_process_thread, NULL);

    while (pbio_busy_count_busy()) {
        pbio_os_run_processes_and_wait_for_event();
    }
}

void pbsys_deinit(void) {

    pbio_os_process_make_request(&pbsys_system_poll_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);

    pbsys_storage_deinit();
    pbsys_hmi_deinit();

    uint32_t start = pbdrv_clock_get_ms();

    // Wait for all relevant pbsys processes to end, but at least 500 ms so we
    // see a shutdown animation even if the button is released sooner.
    while (pbio_busy_count_busy() || pbdrv_clock_get_ms() - start < 500) {
        pbio_os_run_processes_and_wait_for_event();
    }
}

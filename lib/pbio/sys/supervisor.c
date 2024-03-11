// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// System Supervisor

// This file monitors the system health and takes care of things like shutting
// down power on low battery an shutting down motors on overcurrent conditions.

#include <stdbool.h>

#include <pbdrv/watchdog.h>
#include <pbsys/status.h>

#define IDLE_TIME   (3 * 60000)

/**
 * Indicates if the device has been idle for 3 minutes
 */
static inline bool is_idle(void) {
    return pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING, false, IDLE_TIME) &&
           (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING, true, IDLE_TIME) ||
               pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BLUETOOTH_SHUTDOWN, true, IDLE_TIME));
}

/**
 * Polls the system supervisor.
 *
 * This is called periodically to handle any changes in the system state.
 */
void pbsys_supervisor_poll(void) {
    // keep the hub from resetting itself
    pbdrv_watchdog_update();

    // Shut down on low voltage so we don't damage
    // rechargeable batteries, or if the device is idle
    if (is_idle() || pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN, true, 3000)) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
    }
}

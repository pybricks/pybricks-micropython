// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// System Supervisor

// This file monitors the system health and takes care of things like shutting
// down power on low battery an shutting down motors on overcurrent conditions.

#include <stdbool.h>

#include <pbdrv/watchdog.h>
#include <pbsys/status.h>

/**
 * Polls the system supervisor.
 *
 * This is called periodically to handle any changes in the system state.
 */
void pbsys_supervisor_poll(void) {
    // keep the hub from resetting itself
    pbdrv_watchdog_update();

    // Shut down on low voltage so we don't damage rechargeable batteries
    // or if there is no BLE connection made within 3 minutes
    if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN, true, 3000)
        || pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING, true, 3 * 60000)) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN);
    }
}

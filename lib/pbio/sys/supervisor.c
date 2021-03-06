// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// System Supervisor

// This file monitors the system health and takes care of things like shutting
// down power on low battery an shutting down motors on overcurrent conditions.

#include <stdbool.h>

#include <pbdrv/reset.h>
#include <pbsys/status.h>

/**
 * Polls the system supervisor.
 *
 * This is called periodically to handle any changes in the system state.
 */
void pbsys_supervisor_poll(void) {
    // Shut down on low voltage so we don't damage rechargeable batteries
    // or if there is no BLE connection made whithin 30 seconds
    if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN, true, 3000)
        || pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING, true, 3 * 60000)) {
        // REVISIT: this assumes that the power button is not pressed and USB is
        // not plugged in so that the power will actually turn off here
        pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
    }
}

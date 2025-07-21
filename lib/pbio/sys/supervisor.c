// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// System Supervisor

// This file monitors the system health and takes care of things like shutting
// down power on low battery an shutting down motors on overcurrent conditions.

#include <stdbool.h>

#include <pbdrv/watchdog.h>
#include <pbsys/status.h>

#include <pbdrv/bluetooth.h>

/**
 * Polls the system supervisor.
 *
 * This is called periodically to handle any changes in the system state.
 */
void pbsys_supervisor_poll(void) {
    // keep the hub from resetting itself
    pbdrv_watchdog_update();
}

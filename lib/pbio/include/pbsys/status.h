// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup SystemStatus System-wide Status
 * @{
 */

#ifndef _PBSYS_STATUS_H_
#define _PBSYS_STATUS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * System-wide status indications.
 */
typedef enum {
    /** Battery voltage is low. */
    PBSYS_STATUS_BATTERY_LOW_VOLTAGE_WARNING,
    /** Battery voltage is critically low. */
    PBSYS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN,
    /** Battery current is too high. */
    PBSYS_STATUS_BATTERY_HIGH_CURRENT,
    /** Bluetooth Low Energy is advertising/discoverable. */
    PBSYS_STATUS_BLE_ADVERTISING,
    /** Bluetooth Low Energy has low signal. */
    PBSYS_STATUS_BLE_LOW_SIGNAL,
    /** I/O port is busy syncing up with UART device. */
    PBSYS_STATUS_IO_PORT_BUSY,
    /** Power button is currently pressed. */
    PBSYS_STATUS_POWER_BUTTON_PRESSED,
    /** User program is currently running. */
    PBSYS_STATUS_USER_PROGRAM_RUNNING,
    /** Total number of indications. */
    NUM_PBSYS_STATUS,
} pbsys_status_t;

void pbsys_status_set(pbsys_status_t status);
void pbsys_status_clear(pbsys_status_t status);
bool pbsys_status_test(pbsys_status_t status);
bool pbsys_status_test_debounce(pbsys_status_t status, bool state, uint32_t ms);

#endif // _PBSYS_STATUS_H_

/** @} */

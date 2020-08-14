// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * \addtogroup SystemStatus System-wide Status
 * @{
 */

#ifndef _PBSYS_STATUS_H_
#define _PBSYS_STATUS_H_

#include <stdbool.h>

/**
 * System-wide status indication flags.
 */
typedef enum {
    /** Battery voltage is low. */
    PBSYS_STATUS_BATTERY_LOW_VOLTAGE = 1 << 0,
    /** Battery current is too high. */
    PBSYS_STATUS_BATTERY_HIGH_CURRENT = 1 << 1,
    /** Bluetooth Low Energy is advertising/discoverable. */
    PBSYS_STATUS_BLE_ADVERTISING = 1 << 2,
    /** Bluetooth Low Energy has low signal. */
    PBSYS_STATUS_BLE_LOW_SIGNAL = 1 << 3,
    /** I/O port is busy syncing up with UART device. */
    PBSYS_STATUS_IO_PORT_BUSY = 1 << 4,
    /** User program is currently running. */
    PBSYS_STATUS_USER_PROGRAM_RUNNING = 1 << 5,
    /** System power will turn off soon. */
    PBSYS_STATUS_POWER_DOWN_PENDING = 1 << 6,
} pbsys_status_flag_t;

void pbsys_status_set_flag(pbsys_status_flag_t flag);
void pbsys_status_clear_flag(pbsys_status_flag_t flag);
bool pbsys_status_test_flags(pbsys_status_flag_t flags);

#endif // _PBSYS_STATUS_H_

/** @} */

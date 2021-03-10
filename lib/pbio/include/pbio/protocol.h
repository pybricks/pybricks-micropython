// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup ProtocolPybricks Protocol: Pybricks Communication Protocol
 * @{
 */

#ifndef _PBIO_PROTOCOL_H_
#define _PBIO_PROTOCOL_H_

/**
 * Pybricks command types.
 *
 * Commands are sent from a remote device to the hub.
 */
typedef enum {
    PBIO_PYBRICKS_COMMAND_STOP_USER_PROGRAM = 0,
} pbio_pybricks_command_t;

/**
 * Pybricks event types.
 *
 * Events are sent as notifications from the hub to a connected device.
 */
typedef enum {
    /** Status report.*/
    PBIO_PYBRICKS_EVENT_STATUS_REPORT = 0,
} pbio_pybricks_event_t;

/**
 * Hub status indicators.
 *
 * Use PBIO_PYBRICKS_STATUS_FLAG() to convert these value to bit flags if needed.
 */
typedef enum {
    /** Battery voltage is low. */
    PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING = 0,
    /** Battery voltage is critically low. */
    PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN = 1,
    /** Battery current is too high. */
    PBIO_PYBRICKS_STATUS_BATTERY_HIGH_CURRENT = 2,
    /** Bluetooth Low Energy is advertising/discoverable. */
    PBIO_PYBRICKS_STATUS_BLE_ADVERTISING = 3,
    /** Bluetooth Low Energy has low signal. */
    PBIO_PYBRICKS_STATUS_BLE_LOW_SIGNAL = 4,
    /** Power button is currently pressed. */
    PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED = 5,
    /** User program is currently running. */
    PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING = 6,
    /** Total number of indications. */
    NUM_PBIO_PYBRICKS_STATUS,
} pbio_pybricks_status_t;

/**
 * Converts a status value to a bit flag.
 *
 * @param [in]  status  A ::pbio_pybricks_status_t value.
 * @return              A bit flag corresponding to @p status.
 */
#define PBIO_PYBRICKS_STATUS_FLAG(status) (1 << status)

uint32_t pbio_pybricks_event_status_report(uint8_t *buf, uint32_t flags);

#endif // _PBIO_PROTOCOL_H_

/** @} */

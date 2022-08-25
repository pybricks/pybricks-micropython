// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup ProtocolPybricks pbio/protocol: Pybricks Communication Protocol
 * @{
 */

#ifndef _PBIO_PROTOCOL_H_
#define _PBIO_PROTOCOL_H_

#include <stdint.h>

#include <pbio/util.h>

// DEVELOPERS: bump the version as appropriate when adding/changing the protocol
// and use @since tags in the new commands/events/flags.

/** The major version number for the protocol. */
#define PBIO_PROTOCOL_VERSION_MAJOR 1

/** The minor version number for the protocol. */
#define PBIO_PROTOCOL_VERSION_MINOR 1

/** The patch version number for the protocol. */
#define PBIO_PROTOCOL_VERSION_PATCH 0

/**
 * The protocol version as a string.
 *
 * Formatted as "X.Y.Z".
 */
#define PBIO_PROTOCOL_VERSION_STR \
    PBIO_XSTR(PBIO_PROTOCOL_VERSION_MAJOR) "." \
    PBIO_XSTR(PBIO_PROTOCOL_VERSION_MINOR) "." \
    PBIO_XSTR(PBIO_PROTOCOL_VERSION_PATCH)

/**
 * Pybricks command types.
 *
 * Commands are sent from a remote device to the hub.
 */
typedef enum {
    /**
     * Requests that the user program should be stopped.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_COMMAND_STOP_USER_PROGRAM = 0,
} pbio_pybricks_command_t;

/**
 * Pybricks event types.
 *
 * Events are sent as notifications from the hub to a connected device.
 */
typedef enum {
    /**
     * Status report.
     *
     * The payload is a 32-bit little-endian unsigned integer containing
     * ::pbio_pybricks_status_t flags.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_EVENT_STATUS_REPORT = 0,
} pbio_pybricks_event_t;

/**
 * Hub status indicators.
 *
 * Use PBIO_PYBRICKS_STATUS_FLAG() to convert these value to bit flags if needed.
 */
typedef enum {
    /**
     * Battery voltage is low.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING = 0,
    /**
     * Battery voltage is critically low.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN = 1,
    /**
     * Battery current is too high.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_HIGH_CURRENT = 2,
    /**
     * Bluetooth Low Energy is advertising/discoverable.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BLE_ADVERTISING = 3,
    /**
     * Bluetooth Low Energy has low signal.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BLE_LOW_SIGNAL = 4,
    /**
     * Power button is currently pressed.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED = 5,
    /**
     * User program is currently running.
     *
     * @since Protocol v1.0.0
     */
    PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING = 6,
    /**
     * Hub will shut down.
     *
     * @since Protocol v1.1.0
     */
    PBIO_PYBRICKS_STATUS_SHUTDOWN = 7,
    /**
     * Hub shutdown has been requested. System processes may now stop.
     *
     * @since Protocol v1.2.0
     */
    PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST = 8,
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

extern const uint8_t pbio_pybricks_service_uuid[];
extern const uint8_t pbio_pybricks_control_char_uuid[];

extern const uint16_t pbio_gatt_device_info_service_uuid;
extern const uint16_t pbio_gatt_firmware_version_char_uuid;
extern const uint16_t pbio_gatt_software_version_char_uuid;
extern const uint16_t pbio_gatt_pnp_id_char_uuid;

extern const uint8_t pbio_lwp3_hub_service_uuid[];
extern const uint8_t pbio_lwp3_hub_char_uuid[];

extern const uint8_t pbio_nus_service_uuid[];
extern const uint8_t pbio_nus_rx_char_uuid[];
extern const uint8_t pbio_nus_tx_char_uuid[];

// Downloaded programs are received in chunks up to this size.
#define PBIO_PYBRICKS_PROTOCOL_DOWNLOAD_CHUNK_SIZE (100)

#endif // _PBIO_PROTOCOL_H_

/** @} */

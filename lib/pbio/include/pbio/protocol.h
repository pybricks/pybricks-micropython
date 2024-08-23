// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

/**
 * @addtogroup ProtocolPybricks pbio/protocol: Pybricks Communication Profile
 *
 * Additional details about the Pybricks Profile can be found at
 * <https://github.com/pybricks/technical-info/blob/master/pybricks-ble-profile.md>.
 * @{
 */

#ifndef _PBIO_PROTOCOL_H_
#define _PBIO_PROTOCOL_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/util.h>

// DEVELOPERS: bump the version as appropriate when adding/changing the protocol
// and use @since tags in the new commands/events/flags.

/** The major version number for the protocol. */
#define PBIO_PROTOCOL_VERSION_MAJOR 1

/** The minor version number for the protocol. */
#define PBIO_PROTOCOL_VERSION_MINOR 4

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
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_COMMAND_STOP_USER_PROGRAM = 0,

    /**
     * Requests that a specified user program should be started.
     *
     * Parameters:
     * - id    (optional): The identifier of the user program (32-bit little-endian unsigned integer).
     *                     Defaults to 0 if no identifier is provided.
     *                     This optional parameter was introduced in Pybricks Profile v1.4.0
     *
     * Errors:
     * - ::PBIO_PYBRICKS_ERROR_BUSY if a program is already running.
     * - ::PBIO_PYBRICKS_ERROR_INVALID_COMMAND if the builtin program is not available (Since Pybricks Profile v1.4.0).
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_COMMAND_START_USER_PROGRAM = 1,

    /**
     * Requests that a specified builtin program should be started.
     *
     * Parameters:
     * - id    (optional): The identifier of the builtin program (32-bit little-endian unsigned integer).
     *                     Defaults to 0 if no identifier is provided.
     *                     This optional parameter was introduced in Pybricks Profile v1.4.0
     *
     * Errors:
     * - ::PBIO_PYBRICKS_ERROR_BUSY if a program is already running.
     * - ::PBIO_PYBRICKS_ERROR_INVALID_COMMAND if the builtin program is not available (Since Pybricks Profile v1.4.0).
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_COMMAND_START_BUILTIN_PROGRAM = 2,

    /**
     * Requests to write user program metadata.
     *
     * Parameters:
     * - size: The size of the user program in bytes (32-bit little-endian unsigned integer).
     *
     * Errors:
     * - ::PBIO_PYBRICKS_ERROR_BUSY if the user program is running.
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_COMMAND_WRITE_USER_PROGRAM_META = 3,

    /**
     * Requests to write data to user RAM.
     *
     * Parameters:
     * - offset: The offset from the user RAM base address (32-bit little-endian unsigned integer).
     * - payload: The data to write (0 to 507 bytes).
     *
     * Errors:
     * - ::PBIO_PYBRICKS_ERROR_BUSY if the user program is running and the
     *   data would write over the user program area of the user RAM.
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_COMMAND_WRITE_USER_RAM = 4,

    /**
     * Requests for the hub to reboot in firmware update mode.
     *
     * If this command was successful, the hub will reboot immediately, which
     * means the GATT write request will fail because Bluetooth became
     * disconnected.
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_COMMAND_REBOOT_TO_UPDATE_MODE = 5,

    /**
     * Requests to write to stdin on the hub.
     *
     * It is up to the firmware to determine what to with the received data.
     *
     * Parameters:
     * - payload: The data to write (0 to 512 bytes).
     *
     * @since Pybricks Profile v1.3.0
     */
    PBIO_PYBRICKS_COMMAND_WRITE_STDIN = 6,

    /**
     * Requests to write to a buffer that is pre-allocated by a user program.
     *
     * This is typically used by an app such as Pybricks Code to set data that
     * can be polled by a user program.
     *
     * It is up to the user program to determine what to with the received
     * data or how to decode it.
     *
     * Unlike writing to stdin, this data is not queued but overwriten from the
     * given offset.
     *
     * It is up to the sender to ensure that the written data chunks keep the
     * overall data valid, assuming that the user can read it in whole at any
     * time between subsequent writes.
     *
     * Parameters:
     * - offset: The offset from the buffer base address (16-bit little-endian
     *   unsigned integer).
     * - payload: The data to write.
     *
     * @since Pybricks Profile v1.4.0
     */
    PBIO_PYBRICKS_COMMAND_WRITE_APP_DATA = 7,
} pbio_pybricks_command_t;

/**
 * Application-specific error codes that are used in ATT_ERROR_RSP.
 */
typedef enum {
    PBIO_PYBRICKS_ERROR_OK = 0,

    // NB: these values are standard BLE protocol values (i.e. Table 3.4 in v5.3 core specification)

    PBIO_PYBRICKS_ERROR_INVALID_HANDLE = 0x01,
    PBIO_PYBRICKS_ERROR_UNLIKELY_ERROR = 0x0e,
    PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED = 0x13,

    // NB: these values are limited to 0x80 – 0x9F as required by the Bluetooth
    // spec (i.e. Table 3.4 in v5.3 core specification)

    /**
     * An invalid command was requested.
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_ERROR_INVALID_COMMAND = 0x80,
    /**
     * The command cannot be completed now because the required resources are busy.
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_ERROR_BUSY = 0x81,
} pbio_pybricks_error_t;

pbio_pybricks_error_t pbio_pybricks_error_from_pbio_error(pbio_error_t error);

/**
 * Pybricks event types.
 *
 * Events are sent as notifications from the hub to a connected device.
 */
typedef enum {
    /**
     * Status report event.
     *
     * The payload is a 32-bit little-endian unsigned integer containing
     * ::pbio_pybricks_status_t flags.
     *
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_EVENT_STATUS_REPORT = 0,

    /**
     * Data written to stdout event.
     *
     * The payload is a variable number of bytes that was written to stdout.
     *
     * @since Pybricks Profile v1.3.0
     */
    PBIO_PYBRICKS_EVENT_WRITE_STDOUT = 1,

    /**
     * App data sent from the hub to the host. This is similar to stdout, but
     * typically used for data that should not be shown in the user terminal,
     * such as sensor telemetry.
     *
     * The payload is a variable number of bytes that was written to app data.
     *
     * @since Pybricks Profile v1.4.0
     */
    PBIO_PYBRICKS_EVENT_WRITE_APP_DATA = 2,
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
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING = 0,
    /**
     * Battery voltage is critically low.
     *
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN = 1,
    /**
     * Battery current is too high.
     *
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_HIGH_CURRENT = 2,
    /**
     * Bluetooth Low Energy is advertising/discoverable.
     *
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BLE_ADVERTISING = 3,
    /**
     * Bluetooth Low Energy has low signal.
     *
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_STATUS_BLE_LOW_SIGNAL = 4,
    /**
     * Power button is currently pressed.
     *
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED = 5,
    /**
     * User program is currently running.
     *
     * @since Pybricks Profile v1.0.0
     */
    PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING = 6,
    /**
     * Hub will shut down.
     *
     * @since Pybricks Profile v1.1.0
     */
    PBIO_PYBRICKS_STATUS_SHUTDOWN = 7,
    /**
     * Hub shutdown has been requested. System processes may now stop.
     *
     * @since Pybricks Profile v1.2.0
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

/**
 * Application-specific feature flag supported by a hub.
 */
typedef enum {
    // NB: the values are part of the protocol, so don't change the values!

    /**
     * Hub supports builtin programs, such as an interactive REPL or Port View.
     *
     * Prior to version 1.4.0 this flag was exclusively used to indicate REPL
     * support since there were no other builtin programs.
     *
     * @since Pybricks Profile v1.2.0.
     */
    PBIO_PYBRICKS_FEATURE_BUILTIN_PROGRAMS = 1 << 0,
    /**
     * Hub supports user program with multiple MicroPython .mpy files ABI v6
     *
     * Native module support is not implied by this flag but may be combined
     * with additional flags to indicate native module support.
     *
     * @since Pybricks Profile v1.2.0.
     */
    PBIO_PYBRICKS_FEATURE_USER_PROG_FORMAT_MULTI_MPY_V6 = 1 << 1,
    /**
     * Hub supports user program with multiple MicroPython .mpy files ABI v6.1
     * including native module support.
     *
     * @since Pybricks Profile v1.3.0.
     */
    PBIO_PYBRICKS_FEATURE_USER_PROG_FORMAT_MULTI_MPY_V6_1_NATIVE = 1 << 2,
} pbio_pybricks_feature_flags_t;

void pbio_pybricks_hub_capabilities(uint8_t *buf,
    uint16_t max_char_size,
    pbio_pybricks_feature_flags_t feature_flags,
    uint32_t max_user_prog_size);

/**
 * Number of bytes in the Pybricks hub capabilities characteristic value.
 */
#define PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE 10

extern const uint8_t pbio_pybricks_service_uuid[];
extern const uint8_t pbio_pybricks_command_event_char_uuid[];
extern const uint8_t pbio_pybricks_hub_capabilities_char_uuid[];

extern const uint16_t pbio_gatt_device_info_service_uuid;
extern const uint16_t pbio_gatt_firmware_version_char_uuid;
extern const uint16_t pbio_gatt_software_version_char_uuid;
extern const uint16_t pbio_gatt_pnp_id_char_uuid;

extern const uint8_t pbio_nus_service_uuid[];
extern const uint8_t pbio_nus_rx_char_uuid[];
extern const uint8_t pbio_nus_tx_char_uuid[];

/** USB bDeviceClass for Pybricks hubs */
#define PBIO_PYBRICKS_USB_DEVICE_CLASS 0xFF
/** USB bDeviceSubClass for Pybricks hubs */
#define PBIO_PYBRICKS_USB_DEVICE_SUBCLASS 0xC5
/** USB bDeviceProtocol for Pybricks hubs */
#define PBIO_PYBRICKS_USB_DEVICE_PROTOCOL 0xF5

#endif // _PBIO_PROTOCOL_H_

/** @} */

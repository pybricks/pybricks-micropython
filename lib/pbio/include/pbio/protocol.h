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
#define PBIO_PROTOCOL_VERSION_MINOR 5

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
 * User program identifiers.
 *
 *   0--127: Downloadabled user programs.
 * 128--255: Builtin user programs.
 */
typedef enum {
    /**
     * First possible downloadable user program.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_FIRST_SLOT = 0,
    /**
     * Last possible downloadable user program.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_LAST_SLOT = 127,
    /**
     * Read-eval-print loop (REPL) interface.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_REPL = 128,
    /**
     * Program that detects attached devices, displays sensor values, and
     * relays sensor data to host if connected.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_PORT_VIEW = 129,
    /**
     * Program that calibrates the internal inertial measurement unit and saves
     * data persistently on the hub.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_IMU_CALIBRATION = 130,
    /**
     * Program to control EV3 motors with the buttons.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_EV3_MOTOR_BUTTON_CONTROL = 133,
    /**
     * Program to control EV3 motors with infrared remote.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_EV3_MOTOR_IR_CONTROL = 134,
    /**
     * Application to view sensor values.
     */
    PBIO_PYBRICKS_USER_PROGRAM_ID_EV3_PORT_VIEW = 135,
} pbio_pybricks_user_program_id_t;

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
     * Requests that a user program should be started.
     *
     * The optional payload parameter was added in Pybricks Profile v1.4.0.
     *
     * Parameters:
     * - payload: Optional program identifier (one byte). Slots 0--127 are
     *            reserved for downloaded user programs. Slots 128--255 are
     *            for builtin user programs. If no program identifier is
     *            given, the currently active program slot will be started.
     *
     * Errors:
     * - ::PBIO_PYBRICKS_ERROR_BUSY if another program is already running.
     * - ::PBIO_PYBRICKS_ERROR_INVALID_COMMAND if the requested program is not available (since Pybricks Profile v1.4.0.)
     *
     * Errors:
     * - ::PBIO_PYBRICKS_ERROR_BUSY if another program is already running.
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_COMMAND_START_USER_PROGRAM = 1,

    /**
     * Requests that the REPL should be started. This is the same as sending
     * ::PBIO_PYBRICKS_COMMAND_START_USER_PROGRAM with payload ::PBIO_PYBRICKS_USER_PROGRAM_ID_REPL.
     *
     * Errors:
     * - ::PBIO_PYBRICKS_ERROR_BUSY if another program is already running.
     * - ::PBIO_PYBRICKS_ERROR_INVALID_COMMAND if the REPL program is not available.
     *
     * @since Pybricks Profile v1.2.0
     */
    PBIO_PYBRICKS_COMMAND_START_REPL = 2,

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
     * The payload is one 32-bit little-endian unsigned integer containing
     * ::pbio_pybricks_status_flags_t flags and a one byte program identifier
     * representing the currently active program if it is running.
     *
     * @since Pybricks Profile v1.0.0. Program identifier added in Pybricks Profile v1.4.0.
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
     * User data sent from the hub to the host app. This is similar to stdout,
     * but typically used for data that should not be shown in the user
     * terminal.
     *
     * The payload is a variable number of bytes that was written to app data.
     *
     * @since Pybricks Profile v1.4.0
     */
    PBIO_PYBRICKS_EVENT_WRITE_APP_DATA = 2,

    /**
     * Telemetry data sent from the hub to the host.
     *
     * The payload is a variable number of bytes that was written to app data.
     *
     * Pybricks profile version defines exact encoding of this data.
     *
     * @since Unreleased. Should not be considered final.
     */
    PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY = 3,

    /**
     * The total number of events that can be queued and sent.
     */
    PBIO_PYBRICKS_EVENT_NUM_EVENTS,
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
     * Bluetooth Low Energy has low signal. Not implemented or used anywhere.
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
    /**
     * Hub is connected to a host (like Pybricks Code) via BLE.
     *
     * @since Pybricks Profile v1.4.0
     */
    PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED = 9,
    /**
     * Battery temperature is critically high.
     *
     * @since Pybricks Profile v1.5.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_HIGH_TEMP_SHUTDOWN = 10,
    /**
     * Battery temperature is high.
     *
     * @since Pybricks Profile v1.5.0
     */
    PBIO_PYBRICKS_STATUS_BATTERY_HIGH_TEMP_WARNING = 11,
    /**
     * Hub is connected to a host (like Pybricks Code) via USB.
     *
     * @since Pybricks Profile v1.5.0
     */
    PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED = 12,
    /**
     * Hub is receiving, transmitting, or moving a file.
     *
     * @since Pybricks Profile v1.5.0
     */
    PBIO_PYBRICKS_STATUS_FILE_IO_IN_PROGRESS = 13,
    /** Total number of indications. */
    NUM_PBIO_PYBRICKS_STATUS,
} pbio_pybricks_status_flags_t;

/**
 * Converts a status value to a bit flag.
 *
 * @param [in]  status  A ::pbio_pybricks_status_flags_t value.
 * @return              A bit flag corresponding to @p status.
 */
#define PBIO_PYBRICKS_STATUS_FLAG(status) (1 << status)

/** Size of status report event message in bytes. */
#define PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE 7

uint32_t pbio_pybricks_event_status_report(uint8_t *buf, uint32_t flags, pbio_pybricks_user_program_id_t program_id, uint8_t slot);

/**
 * Application-specific feature flag supported by a hub.
 */
typedef enum {
    // NB: the values are part of the protocol, so don't change the values!

    /**
     * Hub supports interactive REPL.
     *
     * @since Pybricks Profile v1.2.0.
     */
    PBIO_PYBRICKS_FEATURE_FLAG_BUILTIN_USER_PROGRAM_REPL = 1 << 0,
    /**
     * Hub supports user program with multiple MicroPython .mpy files ABI v6
     *
     * Native module support is not implied by this flag but may be combined
     * with additional flags to indicate native module support.
     *
     * @since Pybricks Profile v1.2.0.
     */
    PBIO_PYBRICKS_FEATURE_FLAG_USER_PROG_FORMAT_MULTI_MPY_V6 = 1 << 1,
    /**
     * Hub supports user program with multiple MicroPython .mpy files ABI v6.1
     * including native module support.
     *
     * @since Pybricks Profile v1.3.0.
     */
    PBIO_PYBRICKS_FEATURE_FLAG_USER_PROG_FORMAT_MULTI_MPY_V6_1_NATIVE = 1 << 2,
    /**
     * Hub supports builtin sensor port view monitoring program.
     *
     * @since Pybricks Profile v1.4.0.
     */
    PBIO_PYBRICKS_FEATURE_FLAG_BUILTIN_USER_PROGRAM_PORT_VIEW = 1 << 3,
    /**
     * Hub supports builtin IMU calibration program.
     *
     * @since Pybricks Profile v1.4.0.
     */
    PBIO_PYBRICKS_FEATURE_FLAG_BUILTIN_USER_PROGRAM_IMU_CALIBRATION = 1 << 4,
    /**
     * Hub supports user program with multiple MicroPython .mpy files ABI v6.3
     * including native module support.
     *
     * @since Pybricks Profile v1.5.0.
     */
    PBIO_PYBRICKS_FEATURE_FLAG_USER_PROG_FORMAT_MULTI_MPY_V6_3_NATIVE = 1 << 5,
} pbio_pybricks_feature_flags_t;

void pbio_pybricks_hub_capabilities(uint8_t *buf,
    uint16_t max_char_size,
    pbio_pybricks_feature_flags_t feature_flags,
    uint32_t max_user_prog_size,
    uint8_t num_slots);

/**
 * Number of bytes in the Pybricks hub capabilities characteristic value.
 */
#define PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE 11

/**
 * Hub kind identifier.
 *
 * This is the canonical Pybricks identifier for a hub or programmable brick.
 * It is used as the Product ID Field of the Device Information Service PnP ID
 * characteristic (see ::pbio_pybricks_pnp_id) and as the @c device-id field of
 * the firmware metadata.
 *
 * These values are Pybricks-assigned. Coincides with the LWP3 hub kind, but
 * that is a historical convenience only: RCX, NXT and EV3 have no LWP3 hub
 * kind and are assigned values here.
 *
 * Keep in sync with HubType on the host side and with @c PLATFORM_INFO in
 * tools/metadata.py.
 */
typedef enum {
    /** BOOST Move hub. */
    PBIO_PYBRICKS_HUB_KIND_MOVE      = 0x40,
    /** City hub. */
    PBIO_PYBRICKS_HUB_KIND_CITY      = 0x41,
    /** Technic hub. */
    PBIO_PYBRICKS_HUB_KIND_TECHNIC   = 0x80,
    /** SPIKE Prime hub (and its MINDSTORMS Robot Inventor variant). */
    PBIO_PYBRICKS_HUB_KIND_PRIME     = 0x81,
    /** SPIKE Essential hub. */
    PBIO_PYBRICKS_HUB_KIND_ESSENTIAL = 0x83,
    /** MINDSTORMS RCX brick. */
    PBIO_PYBRICKS_HUB_KIND_RCX       = 0xE0,
    /** MINDSTORMS NXT brick. */
    PBIO_PYBRICKS_HUB_KIND_NXT       = 0xE1,
    /** MINDSTORMS EV3 brick. */
    PBIO_PYBRICKS_HUB_KIND_EV3       = 0xE2,
} pbio_pybricks_hub_kind_t;

/**
 * Number of bytes in the Device Information Service PnP ID characteristic value.
 */
#define PBIO_PYBRICKS_PNP_ID_SIZE 7

void pbio_pybricks_pnp_id(uint8_t *buf, uint16_t product_id, uint16_t product_version);

extern const uint8_t pbio_pybricks_service_uuid[];
extern const uint8_t pbio_pybricks_command_event_char_uuid[];
extern const uint8_t pbio_pybricks_hub_capabilities_char_uuid[];

/**
 * Standard Bluetooth GATT UUIDs used as part of the Pybricks "protocol".
 *
 * These are 16-bit UUIDs assigned by the Bluetooth SIG. They are also used to
 * identify the same characteristics over the USB interface.
 */
enum {
    /** Device Information Service UUID. */
    PBIO_GATT_DEVICE_INFO_SERVICE_UUID = 0x180A,
    /** Device Name Characteristic UUID. */
    PBIO_GATT_DEVICE_NAME_CHAR_UUID = 0x2A00,
    /** Firmware Version Characteristic UUID. */
    PBIO_GATT_FIRMWARE_VERSION_CHAR_UUID = 0x2A26,
    /** Software Version Characteristic UUID (Pybricks protocol version). */
    PBIO_GATT_SOFTWARE_VERSION_CHAR_UUID = 0x2A28,
    /** PnP ID Characteristic UUID. */
    PBIO_GATT_PNP_ID_CHAR_UUID = 0x2A50,
};

extern const uint8_t pbio_nus_service_uuid[];
extern const uint8_t pbio_nus_rx_char_uuid[];
extern const uint8_t pbio_nus_tx_char_uuid[];

/**
 * Characteristic namespace for ::PBIO_PYBRICKS_OUT_EP_MSG_READ requests.
 *
 * Selects which group the 16-bit characteristic id in a read request belongs
 * to, mirroring how a BLE host distinguishes standard GATT characteristics
 * from Pybricks-specific ones.
 */
enum {
    /** Retrieve GATT characteristics */
    PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_GATT = 0x01,
    /** Retrieve Pybricks characteristics */
    PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_PYBRICKS = 0x02,
};

// The Pybricks USB interface uses a CDC ACM data pipe (Web Serial on the
// host), which is a raw, bidirectional byte stream with no inherent message
// boundaries. Each direction therefore frames its messages using the SPIKE
// Prime variant of Consistent Overhead Byte Stuffing (COBS). See pbio/cobs.h.
//
// The byte stream is independent of the USB hardware packet size: a single
// frame may span several USB packets, and several small frames may share one
// packet. Only after a frame has been reassembled and decoded is its first
// byte interpreted as the message type below. This is self synchronizing: a
// corrupt or oversized frame is discarded at the next delimiter without losing
// frame alignment.

/**
 * Hub to host message types.
 *
 * The hub to host direction is a single byte stream that multiplexes command
 * responses, events and read replies, so the first byte of each message
 * discriminates between them.
 */
typedef enum {
    /**
     * Reply to a ::PBIO_PYBRICKS_OUT_EP_MSG_COMMAND. The payload is
     * `[tag, status32]`, where `tag` echoes the byte from the command that
     * produced this response (so the host can correlate a late response with
     * the command that produced it) and `status32` is the 32-bit little-endian
     * command error code, analogous to a BLE write response.
     */
    PBIO_PYBRICKS_IN_EP_MSG_RESPONSE = 1,
    /** Analog to BLE notification. Only emitted if subscribed. */
    PBIO_PYBRICKS_IN_EP_MSG_EVENT = 2,
    /**
     * Reply to a ::PBIO_PYBRICKS_OUT_EP_MSG_READ. The payload is
     * `[service, char_id_lo, char_id_hi, value...]`, echoing the selector from
     * the request followed by the characteristic value. An empty value
     * indicates an unknown characteristic.
     */
    PBIO_PYBRICKS_IN_EP_MSG_READ_REPLY = 3,
} pbio_pybricks_usb_in_ep_msg_t;

/**
 * Host to hub message types.
 *
 * The host to hub direction is a single byte stream, so the first byte of each
 * message discriminates between a command and a characteristic read request.
 */
typedef enum {
    /**
     * Subscribe to events. The payload is a single byte: 1 to subscribe, 0 to
     * unsubscribe. The hub will only send events if the host is subscribed.
     *
     * Analog of BLE Client Characteristic Configuration Descriptor (CCCD).
     */
    PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE = 1,
    /**
     * A Pybricks command. The payload is `[tag, ...command]`, where `tag` is a
     * single opaque byte chosen by the host and echoed back in the response,
     * and `command` is a Pybricks command (see ::pbio_pybricks_command_t) with
     * the same encoding as a BLE command write. The hub replies with a
     * ::PBIO_PYBRICKS_IN_EP_MSG_RESPONSE carrying the same tag.
     *
     * The tag lets the host correlate each response with its command, since
     * the bare CDC byte stream has no transaction correlation of its own.
     *
     * Analog of BLE Client Characteristic Write with response.
     */
    PBIO_PYBRICKS_OUT_EP_MSG_COMMAND = 2,
    /**
     * A characteristic read request. The payload is
     * `[service, char_id_lo, char_id_hi]`, where service is one of the
     * ::PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_GATT values. The hub
     * replies with a ::PBIO_PYBRICKS_IN_EP_MSG_READ_REPLY.
     *
     * Analog of a BLE host reading a characteristic by UUID.
     */
    PBIO_PYBRICKS_OUT_EP_MSG_READ = 3,
} pbio_pybricks_usb_out_ep_msg_t;

#endif // _PBIO_PROTOCOL_H_

/** @} */

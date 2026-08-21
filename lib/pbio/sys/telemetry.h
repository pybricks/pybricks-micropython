// Copyright (C) 2026 The Pybricks Authors - All rights reserved
// To build the firmware without this non-free component, set
// PBSYS_CONFIG_TELEMETRY to 0 in pbsysconfig.h.

#ifndef _PBSYS_SYS_TELEMETRY_H_
#define _PBSYS_SYS_TELEMETRY_H_

#include <stdint.h>

#include <pbio/protocol.h>

#include <pbsys/config.h>

// Telemetry message format.
//
// The payload of a ::PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY event consists of
// one or more size-prefixed messages, bundling as many current values as fit.
// Receivers must skip messages they do not understand using the size field,
// so the format can be extended.
//
// Outgoing (event) messages are encoded as follows.
//
// | Offset | Size  | Description                                                        |
// | ------ | ----- | ------------------------------------------------------------------ |
// | 0      | 2     | Message size N (16-bit le), excluding this size field.             |
// | 2      | 1     | Manufacturer: ::pbsys_telemetry_manufacturer_t.                    |
// | 3      | 2     | Device type identifier (16-bit le), scoped to manufacturer.        |
// | 5      | 2     | Location (16-bit le). Meaning is defined per device type, such as port index or data chunk index. ::PBSYS_TELEMETRY_LOCATION_NONE if not applicable. |
// | 7      | 1     | Device mode that the data belongs to.                              |
// | 8      | N - 6 | Mode-specific data.                                                |
//
// The payload of a ::PBIO_PYBRICKS_COMMAND_WRITE_TELEMETRY command is a
// single command: one byte ::pbsys_telemetry_command_t followed by its
// command-specific payload. Each command returns its own success or error.

/**
 * Manufacturer of a telemetry data source.
 */
typedef enum {
    PBSYS_TELEMETRY_MANUFACTURER_LEGO = 0,
} pbsys_telemetry_manufacturer_t;

/**
 * Device family used to compose 16-bit LEGO device type identifiers, occupying
 * the high byte. This is a LEGO-namespace convention only, not protocol
 * structure.
 */
typedef enum {
    PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_POWERED_UP_SENSOR = 0,
    PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_SENSOR = 1,
    PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_BUILTIN = 2,
} pbsys_telemetry_device_family_t;

/**
 * Builtin EV3 devices. Values are scoped to
 * ::PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_BUILTIN.
 */
typedef enum {
    /**
     * Sends packed pixel data in fixed-size chunks, with the location field
     * giving the chunk index. Chunk encoding is defined by the display type.
     */
    PBSYS_TELEMETRY_DEVICE_LEGO_EV3_BUILTIN_DISPLAY = 0,
} pbsys_telemetry_device_lego_ev3_t;

/**
 * Telemetry output level.
 */
typedef enum {
    /** No telemetry output. */
    PBSYS_TELEMETRY_LEVEL_OFF = 0,
    /** Low-bandwidth data such as port values. This is the default. */
    PBSYS_TELEMETRY_LEVEL_MINIMAL = 1,
    /** All data, including large buffers such as the display. */
    PBSYS_TELEMETRY_LEVEL_FULL = 2,
} pbsys_telemetry_level_t;

/**
 * Telemetry commands sent by the host.
 */
typedef enum {
    /**
     * Sets the telemetry output level.
     *
     * Payload: one byte ::pbsys_telemetry_level_t.
     */
    PBSYS_TELEMETRY_COMMAND_SET_LEVEL = 0,
    /**
     * Requests a device mode change.
     *
     * Payload mirrors the outgoing message header: manufacturer, device type
     * identifier, location, and the new mode. Applied only if the given
     * device is still present at that location. Confirmation is implicit via
     * the mode byte of subsequent telemetry data.
     */
    PBSYS_TELEMETRY_COMMAND_SET_MODE = 1,
} pbsys_telemetry_command_t;

/**
 * Composes a 16-bit LEGO device type identifier from family and type.
 */
#define PBSYS_TELEMETRY_DEVICE_ID_LEGO(family, type) ((uint16_t)(((family) << 8) | (type)))

/**
 * Location value for devices where location is not applicable.
 */
#define PBSYS_TELEMETRY_LOCATION_NONE (0xFFFF)

/**
 * Size of the common telemetry message header: manufacturer, device type
 * identifier, location, and mode. Excludes the 16-bit size prefix.
 */
#define PBSYS_TELEMETRY_MSG_HEADER_SIZE (6)

#if PBSYS_CONFIG_TELEMETRY

uint32_t pbsys_telemetry_get_data(uint8_t *data, uint32_t max_size);

pbio_pybricks_error_t pbsys_telemetry_write_data(const uint8_t *data, uint32_t size);

#else

static inline uint32_t pbsys_telemetry_get_data(uint8_t *data, uint32_t max_size) {
    return 0;
}

static inline pbio_pybricks_error_t pbsys_telemetry_write_data(const uint8_t *data, uint32_t size) {
    return PBIO_PYBRICKS_ERROR_INVALID_COMMAND;
}

#endif // PBSYS_CONFIG_TELEMETRY

#endif // _PBSYS_SYS_TELEMETRY_H_

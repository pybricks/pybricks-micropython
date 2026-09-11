// Copyright (C) 2026 The Pybricks Authors - All rights reserved
// To build the firmware without this non-free component, set
// PBSYS_CONFIG_TELEMETRY to 0 in pbsysconfig.h.

#ifndef _PBSYS_TELEMETRY_H_
#define _PBSYS_TELEMETRY_H_

#include <stdint.h>

#include <pbdrv/compiler.h>

#include <pbio/protocol.h>

#include <pbsys/config.h>

/**
 * Telemetry getter return codes, returned by payload generators. This lets us
 * device if we should append a payload and whether to progress in our state
 * machine that goes along each data point. Not part of the protocol.
 */
typedef enum {
    /** Successfully set one payload. */
    PBSYS_TELEMETRY_SUCCESS,
    /** The payload is bigger than available size. */
    PBSYS_TELEMETRY_ERROR_NO_ROOM,
    /** There is nothing new to report. */
    PBSYS_TELEMETRY_ERROR_NO_REPORT,
} pbsys_telemetry_error_t;

/**
 * Telemetry message format.
 *
 * The payload of a ::PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY event consists of
 * one or more size-prefixed messages, bundling as many current values as fit.
 *
 * Outgoing (event) messages are encoded as follows.
 *
 * | Offset | Size  | Description                                                          |
 * | ------ | ----- | ---------------------------------------------------------------------|
 * | 0      | 2     | Message size N (16-bit le), excluding this size field.               |
 * | 2      | 1     | Manufacturer.                                                        |
 * | 3      | 2     | Device type identifier (16-bit le), scoped to manufacturer.          |
 * | 5      | 2     | Device specific location (16-bit le), e.g. port index, pixel offset. |
 * | 7      | 1     | Device mode that the data belongs to.                                |
 * | 8      | N - 6 | Mode-specific data.                                                  |
 */
typedef struct PBDRV_PACKED {
    uint8_t manufacturer;
    uint16_t id;
    uint16_t location;
    uint8_t mode;
    uint8_t payload[];
} pbsys_telemetry_packet_t;

/**
 * Size of the common telemetry message header: manufacturer, device type
 * identifier, location, and mode. Excludes the 16-bit size prefix.
 */
#define PBSYS_TELEMETRY_MSG_HEADER_SIZE (sizeof(pbsys_telemetry_packet_t))

/**
 * Manufacturer of a telemetry data source.
 */
enum {
    PBSYS_TELEMETRY_MANUFACTURER_UKNOWN = 0,
    PBSYS_TELEMETRY_MANUFACTURER_LEGO = 1,
};

/**
 * Generic device IDs for unknown manufacturers.
 */
enum {
    PBSYS_TELEMETRY_DEVICE_UKNOWN_UART = 0,
    PBSYS_TELEMETRY_DEVICE_UKNOWN_I2C = 1,
};

/**
 * Telemetry output level.
 */
typedef enum {
    /** No telemetry output. */
    PBSYS_TELEMETRY_LEVEL_OFF = 0,
    /** Low-bandwidth data such as port values. This is the default. */
    PBSYS_TELEMETRY_LEVEL_MINIMAL = 1,
    /** All data, including large buffers such as the display. */
    PBSYS_TELEMETRY_LEVEL_ALL = 2,
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
     * Payload is a single telemetry message in the outgoing format without
     * the size prefix: header (manufacturer, device type identifier,
     * location, new mode) plus optional mode-specific data. Applied only if
     * the given device is still present at that location. Confirmation is
     * implicit via the mode byte of subsequent telemetry data.
     */
    PBSYS_TELEMETRY_COMMAND_SET_MODE = 1,
} pbsys_telemetry_command_t;

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

#endif // _PBSYS_TELEMETRY_H_

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_TELEMETRY

#include <stdio.h>

#include <pbdrv/display.h>

#include <pbio/os.h>
#include <pbio/port_interface.h>
#include <pbio/util.h>

#include <pbsys/host.h>

#include "telemetry.h"

typedef struct {
    lego_device_type_id_t type_id;
    int32_t value;
} pbsys_telemetry_port_data_t;

static pbsys_telemetry_port_data_t last_data[PBIO_CONFIG_PORT_NUM_DEV];

// REVISIT: Move to shared protocol definitions once the format is settled.

/**
 * Manufacturer of a telemetry data source.
 */
typedef enum {
    PBSYS_TELEMETRY_MANUFACTURER_LEGO = 0,
} pbsys_telemetry_manufacturer_t;

/**
 * Device family within a manufacturer.
 */
typedef enum {
    PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_POWERED_UP_SENSOR = 0,
    PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_SENSOR = 1,
    PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_BUILTIN = 2,
} pbsys_telemetry_device_family_t;

/**
 * Builtin EV3 devices
 */
typedef enum {
    PBSYS_TELEMETRY_DEVICE_LEGO_EV3_BUILTIN_DISPLAY = 0,
} pbsys_telemetry_device_lego_ev3_t;

// Common payload header: manufacturer, family, device type, port id.
#define TELEMETRY_HEADER_SIZE (4)

#define MOTOR_DATA_SIZE (TELEMETRY_HEADER_SIZE + sizeof(uint32_t))

// Display payload: header, start offset, then pixel data in the display
// driver's packed encoding.
#define DISPLAY_DATA_HEADER_SIZE (TELEMETRY_HEADER_SIZE + sizeof(uint32_t))
#define DISPLAY_DATA_MAX_SIZE (500)

// Revisit: Come up with a data encoding protocol. Right now it just sends
// six motor positions to drive the existing motor animation.
static pbio_error_t update_port_data(uint8_t index, uint8_t *buf) {

    // Get type and angle.
    int32_t degrees = 0;
    pbio_angle_t angle;
    lego_device_type_id_t type_id = LEGO_DEVICE_TYPE_ID_NONE;
    pbio_port_t *port = pbio_port_by_index(index);
    pbio_error_t err = pbio_port_get_angle(port, &angle);
    if (err == PBIO_SUCCESS) {
        type_id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
        degrees = pbio_angle_to_low_res(&angle, 1000);
    }

    pbsys_telemetry_port_data_t *data = &last_data[index];

    if (data->type_id == type_id && data->value == degrees) {
        // Same as before, don't send.
        return PBIO_ERROR_AGAIN;
    }

    data->type_id = type_id;
    data->value = degrees;

    buf[0] = PBSYS_TELEMETRY_MANUFACTURER_LEGO;
    buf[1] = PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_POWERED_UP_SENSOR;
    buf[2] = type_id;
    buf[3] = index;
    pbio_set_uint32_le(&buf[4], degrees);
    return PBIO_SUCCESS;
}

#define YIELD_DATA(state, size, return_size) \
    do {                                     \
        *size = (return_size);               \
        PBIO_OS_AWAIT_ONCE(state);           \
    } while (0)

/**
 * The telemetry "process" is not driven from the main event loop, but serves
 * as a data generator, for the host process to pull when ready to send.
 *
 * Uses PBIO_SUCCESS to indicate yielding data, rather than returning.
 */
static pbio_error_t pbsys_telemetry_iterate_data(pbio_os_state_t *state, uint8_t *data, uint32_t *size) {

    // Input argument is how much we are free to write.
    uint32_t available = *size;

    // Resulting yield with 0 means no data, so await.
    *size = 0;

    static uint8_t i = 0;
    static pbio_os_timer_t timer;
    static uint32_t display_offset;
    static uint32_t display_count_reading;
    static uint32_t display_count_sent;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        // Send display frame in chunks, restarting when it changes mid-frame
        // so the receiver always gets full frames of a single update count.
        for (;;) {

            // Can't fit a useful display chunk.
            if (available <= DISPLAY_DATA_HEADER_SIZE) {
                return PBIO_ERROR_BUSY;
            }

            uint32_t max_chunk_size = available - DISPLAY_DATA_HEADER_SIZE;
            if (max_chunk_size > DISPLAY_DATA_MAX_SIZE) {
                max_chunk_size = DISPLAY_DATA_MAX_SIZE;
            }
            uint32_t display_count;
            uint32_t chunk_size = pbdrv_display_get_buffer(&data[DISPLAY_DATA_HEADER_SIZE],
                display_offset, max_chunk_size, &display_count);

            if (display_offset == 0) {
                if (display_count == display_count_sent) {
                    // Nothing new to send.
                    break;
                }
                display_count_reading = display_count;
            } else if (display_count != display_count_reading) {
                // Changed while reading, start over.
                display_offset = 0;
                continue;
            }

            if (chunk_size == 0) {
                // Sent one full frame of the current count.
                display_count_sent = display_count_reading;
                display_offset = 0;
                break;
            }

            data[0] = PBSYS_TELEMETRY_MANUFACTURER_LEGO;
            data[1] = PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_SENSOR;
            data[2] = PBSYS_TELEMETRY_DEVICE_LEGO_EV3_BUILTIN_DISPLAY;
            data[3] = 0;
            pbio_set_uint32_le(&data[4], display_offset);
            display_offset += chunk_size;

            // Yield one display chunk for appending.
            YIELD_DATA(state, size, DISPLAY_DATA_HEADER_SIZE + chunk_size);
        }

        for (i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {

            // Can't fit any more motor samples.
            if (available < MOTOR_DATA_SIZE) {
                return PBIO_ERROR_BUSY;
            }

            // REVISIT: Generalize I/O protocol.
            pbio_error_t err = update_port_data(i, data);
            if (err != PBIO_SUCCESS) {
                // Skip unavailable device.
                continue;
            }

            // Yield one motor payload for appending.
            YIELD_DATA(state, size, MOTOR_DATA_SIZE);
        }

        // Yields with no data.
        PBIO_OS_AWAIT_MS(state, &timer, 40);
    }

    // Unreachable
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

uint32_t pbsys_telemetry_get_data(uint8_t *data, uint32_t max_size) {

    static pbio_os_state_t state;
    uint32_t next_index = 1;

    data[0] = PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY;

    while (next_index < max_size - 2) {

        // Fetch one sensor sample and attempt to append.
        uint32_t size = max_size - next_index - 2;
        pbio_error_t err = pbsys_telemetry_iterate_data(&state, &data[next_index + 2], &size);

        if (err == PBIO_ERROR_AGAIN) {
            // Yield with no data means nothing more now. Send what we have.
            if (!size) {
                break;
            }

            // Got a data point. Advance to the next.
            pbio_set_uint16_le(&data[next_index], size);
            next_index += size + 2;
            continue;
        }

        if (err == PBIO_ERROR_BUSY) {
            // Data full, time to send.
            break;
        }
    }

    return next_index > 1 ? next_index : 0;
}

#endif // PBSYS_CONFIG_TELEMETRY

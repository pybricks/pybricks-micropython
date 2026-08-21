// Copyright (C) 2026 The Pybricks Authors - All rights reserved
// To build the firmware without this non-free component, set
// PBSYS_CONFIG_TELEMETRY to 0 in pbsysconfig.h.

#include <pbsys/config.h>

#if PBSYS_CONFIG_TELEMETRY

#include <stdbool.h>
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

// Telemetry output level, controlled by the host via the set level command.
static pbsys_telemetry_level_t pbsys_telemetry_level = PBSYS_TELEMETRY_LEVEL_FULL;

// Pending mode change per port, requested by the host and to be applied by
// the data generator. Latest request wins.
typedef struct {
    uint16_t device_id;
    uint8_t mode;
    bool pending;
} pbsys_telemetry_pending_mode_t;

static pbsys_telemetry_pending_mode_t pending_modes[PBIO_CONFIG_PORT_NUM_DEV];

#define MOTOR_DATA_SIZE (PBSYS_TELEMETRY_MSG_HEADER_SIZE + sizeof(uint32_t))

#define DISPLAY_DATA_SIZE (PBSYS_TELEMETRY_MSG_HEADER_SIZE + PBDRV_DISPLAY_TELEMETRY_MAX_SIZE)

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
    pbio_set_uint16_le(&buf[1], PBSYS_TELEMETRY_DEVICE_ID_LEGO(
        PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_POWERED_UP_SENSOR, type_id));
    pbio_set_uint16_le(&buf[3], index);
    buf[5] = 0; // Mode. REVISIT: Use actual device mode.
    pbio_set_uint32_le(&buf[6], degrees);
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

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        // Send any new display data, one chunk at a time. The driver tracks
        // read-out progress and is bounded to one frame before yielding.
        while (pbsys_telemetry_level == PBSYS_TELEMETRY_LEVEL_FULL) {

            // Can't fit a display chunk.
            if (available < DISPLAY_DATA_SIZE) {
                return PBIO_ERROR_BUSY;
            }

            uint32_t location;
            uint32_t chunk_size = pbdrv_display_get_telemetry_data(
                &data[PBSYS_TELEMETRY_MSG_HEADER_SIZE], &location);
            if (chunk_size == 0) {
                // Nothing new to send.
                break;
            }

            data[0] = PBSYS_TELEMETRY_MANUFACTURER_LEGO;
            pbio_set_uint16_le(&data[1], PBSYS_TELEMETRY_DEVICE_ID_LEGO(
                PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_BUILTIN,
                PBSYS_TELEMETRY_DEVICE_LEGO_EV3_BUILTIN_DISPLAY));
            pbio_set_uint16_le(&data[3], location);
            data[5] = 0; // Mode.

            // Yield one display chunk for appending.
            YIELD_DATA(state, size, PBSYS_TELEMETRY_MSG_HEADER_SIZE + chunk_size);
        }

        for (i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {

            // REVISIT: Apply pending mode change for this port here.
            (void)pending_modes;

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

    if (pbsys_telemetry_level == PBSYS_TELEMETRY_LEVEL_OFF) {
        return 0;
    }

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

pbio_pybricks_error_t pbsys_telemetry_write_data(const uint8_t *data, uint32_t size) {

    // Payload is a single command: id followed by command-specific payload.
    if (size < 1) {
        return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
    }

    switch (data[0]) {
        case PBSYS_TELEMETRY_COMMAND_SET_LEVEL:
            if (size != 2 || data[1] > PBSYS_TELEMETRY_LEVEL_FULL) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }
            pbsys_telemetry_level = data[1];
            return PBIO_PYBRICKS_ERROR_OK;
        case PBSYS_TELEMETRY_COMMAND_SET_MODE: {
            // Command id followed by the outgoing message header.
            if (size != 1 + PBSYS_TELEMETRY_MSG_HEADER_SIZE) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }
            if (data[1] != PBSYS_TELEMETRY_MANUFACTURER_LEGO) {
                // Unknown manufacturer, ignore.
                return PBIO_PYBRICKS_ERROR_OK;
            }
            uint16_t location = pbio_get_uint16_le(&data[4]);
            if (location >= PBIO_CONFIG_PORT_NUM_DEV) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }
            // Latest request wins. Applied by the data generator.
            pending_modes[location] = (pbsys_telemetry_pending_mode_t) {
                .device_id = pbio_get_uint16_le(&data[2]),
                .mode = data[6],
                .pending = true,
            };
            return PBIO_PYBRICKS_ERROR_OK;
        }
        default:
            return PBIO_PYBRICKS_ERROR_INVALID_COMMAND;
    }
}

#endif // PBSYS_CONFIG_TELEMETRY

// Copyright (C) 2026 The Pybricks Authors - All rights reserved
// To build the firmware without this non-free component, set
// PBSYS_CONFIG_TELEMETRY to 0 in pbsysconfig.h.

#include <pbsys/config.h>

#if PBSYS_CONFIG_TELEMETRY

#include <stdbool.h>
#include <stdio.h>

#include <pbdrv/display.h>

#include <pbio/os.h>
#include <pbio/port.h>
#include <pbio/util.h>

#include <pbsys/host.h>

#include <pbsys/telemetry.h>

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


/**
 * The telemetry "process" is not driven from the main event loop, but serves
 * as a data generator, for the host process to pull when ready to send.
 *
 * Uses PBIO_SUCCESS to indicate yielding data, rather than returning.
 */
static pbio_error_t pbsys_telemetry_iterate_data(pbio_os_state_t *state, uint8_t *data, uint32_t *size) {

    // Input argument is how much we are free to write.
    uint32_t available = PBIO_OS_YIELD_DATA_INIT(size);

    // Should be able to write at least a header.
    if (available <= PBSYS_TELEMETRY_MSG_HEADER_SIZE) {
        return PBIO_ERROR_BUSY;
    }

    static uint8_t i = 0;
    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        // Send any new display data, one chunk at a time. The driver tracks
        // read-out progress and is bounded to one frame before yielding.
        if (pbsys_telemetry_level == PBSYS_TELEMETRY_LEVEL_FULL) {

            for (;;) {

                uint32_t display_write_size = available - PBSYS_TELEMETRY_MSG_HEADER_SIZE;
                uint32_t location;

                static pbio_os_state_t display_state;
                pbio_error_t err = pbdrv_display_iterate_data(&display_state, data + PBSYS_TELEMETRY_MSG_HEADER_SIZE, &display_write_size, &location);

                if (err == PBIO_ERROR_BUSY) {
                    // Not enough room now. Send what we had already and come back later.
                    return err;
                }

                if (err != PBIO_ERROR_AGAIN) {
                    // Full frame done or nothing new to send, move on.
                    break;
                }

                if (display_write_size) {
                    // We got a chunk, so queue it for sending.
                    data[0] = PBSYS_TELEMETRY_MANUFACTURER_LEGO;
                    pbio_set_uint16_le(&data[1], PBSYS_TELEMETRY_DEVICE_ID_LEGO(
                        PBSYS_TELEMETRY_DEVICE_FAMILY_LEGO_EV3_BUILTIN,
                        PBSYS_TELEMETRY_DEVICE_LEGO_EV3_BUILTIN_DISPLAY));
                    pbio_set_uint16_le(&data[3], location);
                    data[5] = 0; // Mode.

                    // Yield one display chunk for appending.
                    PBIO_OS_YIELD_DATA(state, size, PBSYS_TELEMETRY_MSG_HEADER_SIZE + display_write_size);
                }
            }
        }

        (void)i;

        for (i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {

            pbsys_telemetry_error_t terr = pbio_port_get_telemetry(i, data, &available);
            if (terr == PBSYS_TELEMETRY_ERROR_NO_ROOM) {
                return PBIO_ERROR_BUSY;
            }

            // REVISIT: Apply pending mode change for this port here.
            // and handle not ready
            (void)pending_modes;

            // Yield one motor payload for appending.
            PBIO_OS_YIELD_DATA(state, size, available);
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

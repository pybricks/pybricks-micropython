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

// Got one sample, yield for appending.
#define PBSYS_TELEMETRY_YIELD(state)                 \
    do {                                             \
        do_yield_now = 1;                            \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);         \
        if (do_yield_now) {                          \
            return true;                             \
        }                                            \
    } while (0)

// No room or resource to append further for now. Send what we have.
#define PBSYS_TELEMETRY_FULL(state) return false;

// Idle the generator.
#define PBSYS_TELEMETRY_IDLE(state, timer, duration)  \
    do {                                              \
        pbio_os_timer_set(timer, duration);           \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);          \
        if (!pbio_os_timer_is_expired(timer)) {       \
            return false;                             \
        }                                             \
    } while (0)


static bool pbsys_telemetry_iterate_data(pbsys_telemetry_packet_t *tel, uint32_t *size) {

    static uint8_t i = 0;
    static pbio_os_timer_t timer;
    static pbio_os_state_t state;

    pbsys_telemetry_error_t terr;

    PBIO_OS_ASYNC_BEGIN(&state);

    for (;;) {

        // Send any new display data, one chunk at a time. The driver tracks
        // read-out progress and is bounded to one frame before yielding.
        if (pbsys_telemetry_level == PBSYS_TELEMETRY_LEVEL_FULL) {
            for (;;) {
                terr = pbdrv_display_iterate_data(tel, size);
                if (terr == PBSYS_TELEMETRY_ERROR_NO_ROOM) {
                    // Not enough room now. Send what we had already and come back later.
                    PBSYS_TELEMETRY_FULL();
                } else if (terr == PBSYS_TELEMETRY_ERROR_NO_REPORT) {
                    // Nothing new to send from this iterator, move on.
                    break;
                } else if (terr == PBSYS_TELEMETRY_ERROR_PARTIAL) {
                    PBSYS_TELEMETRY_YIELD(&state);
                    // Resume for more chunks.
                    continue;
                } else if (terr == PBSYS_TELEMETRY_SUCCESS) {
                    PBSYS_TELEMETRY_YIELD(&state);
                    // Last chunk, move on.
                    break;
                }
            }
        }

        // Poll ports in order.
        for (i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {

            terr = pbio_port_get_telemetry(i, tel, size);

            if (terr == PBSYS_TELEMETRY_ERROR_NO_ROOM) {
                // Variable sized payload won't fit this time.
                PBSYS_TELEMETRY_FULL();
            }
            if (terr == PBSYS_TELEMETRY_ERROR_NO_REPORT) {
                // This port has nothing new to say.
                continue;
            }
            if (terr == PBSYS_TELEMETRY_SUCCESS) {
                // Did get one sample, yield for appending.
                PBSYS_TELEMETRY_YIELD(&state);
            }

            // REVISIT: Apply pending mode change for this port here.
            // and handle not ready
            (void)pending_modes;
        }

        // Idle between sequences of samples.
        PBSYS_TELEMETRY_IDLE(&state, &timer, 40);
    }

    // Unreachable
    PBIO_OS_ASYNC_END(false);
}

uint32_t pbsys_telemetry_get_data(uint8_t *data, uint32_t max_size) {

    uint32_t next_index = 1;

    if (pbsys_telemetry_level == PBSYS_TELEMETRY_LEVEL_OFF) {
        return 0;
    }

    data[0] = PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY;

    while (next_index + sizeof(uint16_t) + PBSYS_TELEMETRY_MSG_HEADER_SIZE <= max_size) {

        // Fetch one sensor sample and attempt to append.
        uint32_t size = max_size - next_index - sizeof(uint16_t);

        pbsys_telemetry_packet_t *tel = (pbsys_telemetry_packet_t *)&data[next_index + sizeof(uint16_t)];

        // Attempt to get next data point.
        if (!pbsys_telemetry_iterate_data(tel, &size)) {
            // Data full or idle, time to send.
            break;
        }

        // Got a data point. Advance to the next.
        pbio_set_uint16_le(&data[next_index], size + PBSYS_TELEMETRY_MSG_HEADER_SIZE);
        next_index += size + sizeof(uint16_t) + PBSYS_TELEMETRY_MSG_HEADER_SIZE;
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

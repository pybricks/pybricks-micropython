// Copyright (C) 2026 The Pybricks Authors - All rights reserved
// To build the firmware without this non-free component, set
// PBSYS_CONFIG_TELEMETRY to 0 in pbsysconfig.h.

#include <pbsys/config.h>

#if PBSYS_CONFIG_TELEMETRY

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/display.h>

#include <pbio/os.h>
#include <pbio/port.h>
#include <pbio/util.h>

#include <pbsys/host.h>

#include <pbsys/telemetry.h>

// Telemetry output level, controlled by the host via the set level command.
static pbsys_telemetry_level_t pbsys_telemetry_level = PBSYS_TELEMETRY_LEVEL_MINIMAL;

// Maximum mode-specific payload size accepted for incoming set mode commands.
// None of the current commands need a payload, so keep it small.
#define PBSYS_TELEMETRY_SET_MODE_PAYLOAD_MAX (4)

// Pending mode change per port, requested by the host and to be applied by
// the data generator. Latest request wins.
static struct {
    // Message size including header. 0 means no pending request.
    uint8_t size;
    union {
        pbsys_telemetry_packet_t tel;
        // Reserves room for the flexible payload of tel.
        uint8_t buf[PBSYS_TELEMETRY_MSG_HEADER_SIZE + PBSYS_TELEMETRY_SET_MODE_PAYLOAD_MAX];
    };
} pending_modes[PBIO_CONFIG_PORT_NUM_DEV];

// Yields one report if the getter produced one, skips to the next stage if it
// had nothing, or sends what we have and retries the same getter when out of
// room.
#define PBSYS_TELEMETRY_STAGE(state, call)             \
    do {                                               \
        pbsys_telemetry_error_t terr;                  \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);           \
        terr = (call);                                 \
        if (terr == PBSYS_TELEMETRY_ERROR_NO_ROOM) {   \
            return false;                              \
        }                                              \
        if (terr == PBSYS_TELEMETRY_SUCCESS) {         \
            do_yield_now = 1;                          \
            PBIO_OS_ASYNC_SET_CHECKPOINT(state);       \
            if (do_yield_now) {                        \
                return true;                           \
            }                                          \
        }                                              \
    } while (0)

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

    PBIO_OS_ASYNC_BEGIN(&state);

    for (;;) {

        // Send any new display data, one chunk at a time. The driver says when
        // to stop, so other stages get their turn on a busy display.
        if (pbsys_telemetry_level == PBSYS_TELEMETRY_LEVEL_ALL) {
            // Static to survive yields between chunks.
            static bool display_done;
            for (display_done = false; !display_done;) {
                PBSYS_TELEMETRY_STAGE(&state, pbdrv_display_iterate_data(tel, &display_done, size));
            }
        }

        // Poll ports in order.
        for (i = 0; i < PBIO_CONFIG_PORT_NUM_DEV; i++) {

            PBSYS_TELEMETRY_STAGE(&state, pbio_port_get_telemetry(i, tel, size));

            // Apply pending mode change. Ignore failure; confirmation is
            // implicit via the mode byte of subsequent telemetry data.
            if (pending_modes[i].size) {
                pbio_port_set_telemetry_mode(i, &pending_modes[i].tel, pending_modes[i].size - PBSYS_TELEMETRY_MSG_HEADER_SIZE);
                pending_modes[i].size = 0;
            }
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
        uint32_t room = max_size - next_index - sizeof(uint16_t) - PBSYS_TELEMETRY_MSG_HEADER_SIZE;
        uint32_t size = room;

        pbsys_telemetry_packet_t *tel = (pbsys_telemetry_packet_t *)&data[next_index + sizeof(uint16_t)];

        // Attempt to get next data point.
        if (!pbsys_telemetry_iterate_data(tel, &size)) {
            // Data full or idle, time to send.
            break;
        }

        // A generator that reports more than it was given has already written
        // out of bounds, so stop rather than compound it by advancing past
        // the end of the buffer.
        if (size > room) {
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
            if (size != 2 || data[1] > PBSYS_TELEMETRY_LEVEL_ALL) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }
            pbsys_telemetry_level = data[1];
            return PBIO_PYBRICKS_ERROR_OK;
        case PBSYS_TELEMETRY_COMMAND_SET_MODE: {
            // Command id followed by one telemetry message: the outgoing
            // message format without the size prefix, so header + payload.
            if (size < 1 + PBSYS_TELEMETRY_MSG_HEADER_SIZE ||
                size > 1 + PBSYS_TELEMETRY_MSG_HEADER_SIZE + PBSYS_TELEMETRY_SET_MODE_PAYLOAD_MAX) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }
            const pbsys_telemetry_packet_t *tel = (const pbsys_telemetry_packet_t *)&data[1];
            if (tel->manufacturer != PBSYS_TELEMETRY_MANUFACTURER_LEGO) {
                // Unknown manufacturer, ignore.
                return PBIO_PYBRICKS_ERROR_OK;
            }
            if (tel->location >= PBIO_CONFIG_PORT_NUM_DEV) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }
            // Latest request wins. Applied by the data generator.
            memcpy(&pending_modes[tel->location].tel, tel, size - 1);
            pending_modes[tel->location].size = size - 1;
            return PBIO_PYBRICKS_ERROR_OK;
        }
        default:
            return PBIO_PYBRICKS_ERROR_INVALID_COMMAND;
    }
}

#endif // PBSYS_CONFIG_TELEMETRY

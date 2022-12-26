// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <string.h>

#include <pbdrv/clock.h>

#include <pbio/broadcast.h>
#include <pbio/error.h>
#include <pbio/util.h>

#if PBIO_CONFIG_BROADCAST_NUM_SIGNALS != 0

#include <pbdrv/bluetooth.h>

static struct {
    pbdrv_bluetooth_value_t value;
    uint8_t header[3];
    uint8_t index;
    uint32_t hash;
    char payload[PBIO_BROADCAST_MAX_PAYLOAD_SIZE];
    uint32_t timestamp;
} __attribute__((packed)) transmit_signal = {
    // Header for compatibility with official LEGO MINDSTORMS App:
    // 0xFF for manufacturer data and 0x0397 for company ID.
    .header = {255, 3, 151},
};

#define PBIO_BROADCAST_META_SIZE (8)
#define PBIO_BROADCAST_MAX_PAYLOAD_SIZE (23)
#define PBIO_BROADCAST_DELAY_REPEAT_MS (100)

// Received signals.
typedef struct _pbio_broadcast_received_t {
    uint32_t timestamp;
    uint8_t size;
    uint8_t index;
    uint32_t hash;
    uint8_t payload[PBIO_BROADCAST_MAX_PAYLOAD_SIZE];
} pbio_broadcast_received_t;

pbio_broadcast_received_t received_signals[PBIO_CONFIG_BROADCAST_NUM_SIGNALS];

// Number of signals we are scanning for.
static uint8_t num_scan_signals;

void pbio_broadcast_clear_all(void) {

    // Clear transmitted signal.
    transmit_signal.value.size = 0;

    // Clear number of signals we are scanning for.
    num_scan_signals = 0;
}

pbio_error_t pbio_broadcast_register_signal(uint32_t hash) {
    // Check if there are any slots left.
    if (num_scan_signals == PBIO_CONFIG_BROADCAST_NUM_SIGNALS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Go through all signals to see if this hash is already in use.
    for (uint8_t i = 0; i < num_scan_signals; i++) {
        if (received_signals[i].hash == hash) {
            return PBIO_ERROR_INVALID_ARG;
        }
    }

    // Get the next available signal and increment the number of used slots.
    pbio_broadcast_received_t *signal = &received_signals[num_scan_signals++];

    // Reset the signal.
    signal->hash = hash;
    signal->index = 0;
    signal->size = 0;

    return PBIO_SUCCESS;
}

void pbio_broadcast_receive(uint32_t hash, uint8_t **payload, uint8_t *size) {
    for (uint8_t i = 0; i < num_scan_signals; i++) {

        pbio_broadcast_received_t *s = &received_signals[i];

        // Return if there is a match.
        if (s->hash == hash) {
            *payload = s->payload;
            *size = s->size;
            return;
        }
    }

    // No signal registered for this hash, so size is 0.
    *size = 0;
}

void pbio_broadcast_transmit(uint32_t hash, const uint8_t *payload, uint8_t size) {

    // Cut off payloads that are too long.
    if (size > PBIO_BROADCAST_MAX_PAYLOAD_SIZE) {
        size = PBIO_BROADCAST_MAX_PAYLOAD_SIZE;
    }

    uint8_t old_payload_size = transmit_signal.value.size - PBIO_BROADCAST_META_SIZE;
    uint32_t time_now = pbdrv_clock_get_ms();

    // If we've sent the same message recently, there is nothing we need to do.
    if (transmit_signal.hash == hash &&
        old_payload_size == size &&
        time_now - transmit_signal.timestamp < PBIO_BROADCAST_DELAY_REPEAT_MS &&
        !memcmp(transmit_signal.payload, payload, size)) {
        return;
    }

    // If new the message is on the same signal, increment the index.
    if (transmit_signal.hash == hash) {
        transmit_signal.index++;
    } else {
        // Otherwise reset the index.
        transmit_signal.index = 0;
        transmit_signal.hash = hash;
    }

    // Copy the payload.
    transmit_signal.timestamp = time_now;
    transmit_signal.value.size = size + PBIO_BROADCAST_META_SIZE;
    memcpy(transmit_signal.payload, payload, size);

    // Also make transmitted signal readable by itself.
    pbio_broadcast_parse_advertising_data(transmit_signal.value.data, transmit_signal.value.size);

    // Start broadcasting it.
    pbdrv_bluetooth_start_data_advertising(&transmit_signal.value);
}

void pbio_broadcast_parse_advertising_data(const uint8_t *data, uint8_t size) {

    // Return immediately for programs that don't use broadcast.
    if (!num_scan_signals || size < PBIO_BROADCAST_META_SIZE) {
        return;
    }

    // We only process data with the right header
    if (memcmp(data, &transmit_signal.header[0], 3)) {
        return;
    }

    // Go through signal candidates to find a match.
    for (uint8_t i = 0; i < num_scan_signals; i++) {
        pbio_broadcast_received_t *signal = &received_signals[i];

        // If received does not match registered hash, skip it.
        if (pbio_get_uint32_le(&data[4]) != signal->hash) {
            continue;
        }

        // Get time and idex for incoming signal.
        uint32_t time_now = pbdrv_clock_get_ms();
        uint8_t index_now = data[3];

        // For very quick updates, we should skip updating if new index is older.
        if (time_now - signal->timestamp < PBIO_BROADCAST_DELAY_REPEAT_MS && index_now - signal->index > 128) {
            continue;
        }

        // We have a match, so store the signal and return.
        signal->size = size - PBIO_BROADCAST_META_SIZE;
        signal->index = index_now;
        signal->timestamp = time_now;
        memcpy(signal->payload, &data[PBIO_BROADCAST_META_SIZE], signal->size);
        return;
    }
}

#endif // PBIO_CONFIG_BROADCAST_NUM_SIGNALS

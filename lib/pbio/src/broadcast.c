// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <string.h>

#include <contiki.h>

#include <pbdrv/clock.h>

#include <pbio/broadcast.h>
#include <pbio/error.h>
#include <pbio/util.h>

#if PBIO_CONFIG_BROADCAST_NUM_SIGNALS != 0

#include <pbdrv/bluetooth.h>

static struct {
    // Message data
    pbdrv_bluetooth_value_t value;
    uint8_t length;
    uint8_t AD_type;
    uint16_t company_id;
    uint8_t index;
    uint32_t hash;
    char payload[PBIO_BROADCAST_MAX_PAYLOAD_SIZE];
    // Transmission info
    uint32_t timestamp;
    bool advertising_now;
} __attribute__((packed)) transmit_signal = {
    .AD_type = 0xff, // manufacturer data
    .company_id = LWP3_LEGO_COMPANY_ID, // for compatibility with official LEGO MINDSTORMS App
    .advertising_now = false,
};

#define PBIO_BROADCAST_META_SIZE (9)
#define PBIO_BROADCAST_DELAY_REPEAT_MS (100)

PROCESS(pbio_broadcast_process, "broadcast");

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

    // Poll to start broadcasting process
    process_poll(&pbio_broadcast_process);
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
    transmit_signal.length = size - 1;
    transmit_signal.timestamp = time_now;
    transmit_signal.value.size = size + PBIO_BROADCAST_META_SIZE;
    memcpy(transmit_signal.payload, payload, size);

    // Also make transmitted signal readable by itself.
    pbio_broadcast_parse_advertising_data(transmit_signal.value.data, transmit_signal.value.size);

    // Prepare to start broadcasting it.
    process_poll(&pbio_broadcast_process);
}

void pbio_broadcast_parse_advertising_data(const uint8_t *data, uint8_t size) {

    // Return immediately for programs that don't use broadcast.
    if (!num_scan_signals || size < PBIO_BROADCAST_META_SIZE) {
        return;
    }

    // We only process data with the right header
    if ((data[0] != size - 1) && data[1] != transmit_signal.AD_type &&
        memcmp(&data[2], &transmit_signal.company_id, 2)) {
        return;
    }

    // Go through signal candidates to find a match.
    for (uint8_t i = 0; i < num_scan_signals; i++) {
        pbio_broadcast_received_t *signal = &received_signals[i];

        // If received does not match registered hash, skip it.
        if (pbio_get_uint32_le(&data[5]) != signal->hash) {
            continue;
        }

        // Get time and idex for incoming signal.
        uint32_t time_now = pbdrv_clock_get_ms();
        uint8_t index_now = data[4];

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

PROCESS_THREAD(pbio_broadcast_process, ev, data) {
    static struct etimer timer;

    static pbio_task_t task;

    PROCESS_BEGIN();

    // Revisit: Currently we require a poll to enter the broadcast loop and
    // we also don't stop properly.
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

    etimer_set(&timer, 1000);

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL || (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)));

        // Check which condition triggered the update.
        if (ev == PROCESS_EVENT_POLL) {
            // Poll, so we must start sending new data
            pbdrv_bluetooth_set_advertising_data(&task, &transmit_signal.value);
            PROCESS_WAIT_UNTIL(task.status == PBIO_SUCCESS);

            // Start advertising if we are not already.
            if (!transmit_signal.advertising_now) {
                pbdrv_bluetooth_start_data_advertising(&task);
                PROCESS_WAIT_UNTIL(task.status == PBIO_SUCCESS);

                transmit_signal.advertising_now = true;
            }

            // Reset timer
            etimer_restart(&timer);
        } else {
            // Otherwise, the timer has expired, so stop transmitting.
            pbdrv_bluetooth_stop_data_advertising(&task);
            PROCESS_WAIT_UNTIL(task.status == PBIO_SUCCESS);

            transmit_signal.advertising_now = false;
        }

    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_BROADCAST_NUM_SIGNALS

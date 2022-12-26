// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_BROADCAST_H_
#define _PBIO_BROADCAST_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>

#if PBIO_CONFIG_BROADCAST_NUM_SIGNALS != 0

#define PBIO_BROADCAST_META_SIZE (8)
#define PBIO_BROADCAST_MAX_PAYLOAD_SIZE (23)
#define PBIO_BROADCAST_DELAY_REPEAT_MS (100)

typedef struct _pbio_broadcast_received_t {
    uint32_t timestamp;
    uint8_t size;
    uint8_t index;
    uint32_t hash;
    uint8_t payload[PBIO_BROADCAST_MAX_PAYLOAD_SIZE];
} pbio_broadcast_received_t;

void pbio_broadcast_parse_advertising_data(const uint8_t *data, uint8_t size);

void pbio_broadcast_clear_all(void);

pbio_error_t pbio_broadcast_register_signal(uint32_t hash);

pbio_error_t pbio_broadcast_get_signal(pbio_broadcast_received_t **signal, uint32_t hash);

void pbio_broadcast_transmit(uint32_t hash, const uint8_t *payload, uint8_t size);

#else

static inline void pbio_broadcast_parse_advertising_data(const uint8_t *data, uint8_t size) {
}

static inline void pbio_broadcast_clear_all(void) {
}

#endif // PBIO_CONFIG_BROADCAST_NUM_SIGNALS

#endif // _PBIO_BROADCAST_H_

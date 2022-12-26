// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_BROADCAST_H_
#define _PBIO_BROADCAST_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>

#if PBIO_CONFIG_BROADCAST_NUM_SIGNALS != 0

#define PBIO_BROADCAST_MAX_PAYLOAD_SIZE (23)

void pbio_broadcast_parse_advertising_data(const uint8_t *data, uint8_t size);

void pbio_broadcast_clear_all(void);

pbio_error_t pbio_broadcast_register_signal(uint32_t hash);

void pbio_broadcast_receive(uint32_t hash, uint8_t **payload, uint8_t *size);

void pbio_broadcast_transmit(uint32_t hash, const uint8_t *payload, uint8_t size);

#else

static inline void pbio_broadcast_parse_advertising_data(const uint8_t *data, uint8_t size) {
}

static inline void pbio_broadcast_clear_all(void) {
}

#endif // PBIO_CONFIG_BROADCAST_NUM_SIGNALS

#endif // _PBIO_BROADCAST_H_

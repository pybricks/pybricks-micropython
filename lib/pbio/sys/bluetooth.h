// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _PBSYS_SYS_BLUETOOTH_H_
#define _PBSYS_SYS_BLUETOOTH_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/error.h>
#include <pbsys/host.h>

void pbsys_bluetooth_process_poll(void);

#if PBSYS_CONFIG_BLUETOOTH

void pbsys_bluetooth_init(void);
pbio_error_t pbsys_bluetooth_tx(const uint8_t *data, uint32_t *size);
bool pbsys_bluetooth_tx_is_idle(void);

#else // PBSYS_CONFIG_BLUETOOTH

#define pbsys_bluetooth_init()

static inline pbio_error_t pbsys_bluetooth_tx(const uint8_t *data, uint32_t *size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline bool pbsys_bluetooth_tx_is_idle(void) {
    return false;
}

#endif // PBSYS_CONFIG_BLUETOOTH

#endif // _PBSYS_SYS_BLUETOOTH_H_

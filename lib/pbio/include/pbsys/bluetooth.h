// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup SystemBluetooth System: Bluetooth
 * @{
 */

#ifndef _PBSYS_BLUETOOTH_H_
#define _PBSYS_BLUETOOTH_H_

#include <pbsys/config.h>
#include <pbio/error.h>

#if PBSYS_CONFIG_BLUETOOTH

#include <stdint.h>

#include <pbsys/user_program.h>

void pbsys_bluetooth_init(void);
void pbsys_bluetooth_rx_set_callback(pbsys_user_program_stdin_event_callback_t callback);
void pbsys_bluetooth_rx_flush(void);
uint32_t pbsys_bluetooth_rx_get_available(void);
pbio_error_t pbsys_bluetooth_rx(uint8_t *data, uint32_t *size);
pbio_error_t pbsys_bluetooth_tx(const uint8_t *data, uint32_t *size);

#else // PBSYS_CONFIG_BLUETOOTH

#define pbsys_bluetooth_init()
#define pbsys_bluetooth_rx_set_callback(callback)
#define pbsys_bluetooth_rx_flush()
#define pbsys_bluetooth_rx_get_available() 0

static inline pbio_error_t pbsys_bluetooth_rx(uint8_t *data, uint32_t *size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_bluetooth_tx(const uint8_t *data, uint32_t *size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBSYS_CONFIG_BLUETOOTH

#endif // _PBSYS_BLUETOOTH_H_

/** @} */

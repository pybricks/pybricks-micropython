// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

/**
 * @addtogroup SystemBluetooth System: Bluetooth
 * @{
 */

#ifndef _PBSYS_BLUETOOTH_H_
#define _PBSYS_BLUETOOTH_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/error.h>
#include <pbsys/config.h>

/**
 * Callback function to handle stdin events.
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be placed
 *                  in the stdin buffer, otherwise *false*.
 */
typedef bool (*pbsys_bluetooth_stdin_event_callback_t)(uint8_t c);

#if PBSYS_CONFIG_BLUETOOTH

void pbsys_bluetooth_init(void);
void pbsys_bluetooth_rx_set_callback(pbsys_bluetooth_stdin_event_callback_t callback);
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

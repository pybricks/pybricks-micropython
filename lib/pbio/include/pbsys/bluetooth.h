// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup SystemBluetooth System: Bluetooth
 * @{
 */

#ifndef _PBSYS_BLUETOOTH_H_
#define _PBSYS_BLUETOOTH_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbsys/sys.h>

void pbsys_bluetooth_init(void);
void pbsys_bluetooth_rx_set_callback(pbsys_stdin_event_callback_t callback);
uint32_t pbsys_bluetooth_rx_get_available(void);
pbio_error_t pbsys_bluetooth_rx(uint8_t *data, uint32_t *size);
pbio_error_t pbsys_bluetooth_tx(const uint8_t *data, uint32_t *size);

#endif // _PBSYS_BLUETOOTH_H_

/** @} */

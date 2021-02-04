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

void pbsys_bluetooth_init(void);
pbio_error_t pbsys_bluetooth_tx(uint8_t c);

#endif // _PBSYS_BLUETOOTH_H_

/** @} */

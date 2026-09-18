// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BLUETOOTH_NXT_H_
#define _INTERNAL_PBDRV_BLUETOOTH_NXT_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_NXT

/**
 * Gets the Bluetooth address of this hub. It is the only identifier the NXT
 * has, so it doubles as the USB serial number.
 *
 * It has to be read from the BlueCore 4, so it is not available until that
 * chip has booted.
 *
 * @param [out] addr  The 6-byte address, unchanged if not available yet.
 * @return            Whether the address was available.
 */
bool pbdrv_bluetooth_nxt_get_local_address(uint8_t *addr);

#else // PBDRV_CONFIG_BLUETOOTH_NXT

static inline bool pbdrv_bluetooth_nxt_get_local_address(uint8_t *addr) {
    return false;
}

#endif // PBDRV_CONFIG_BLUETOOTH_NXT

#endif // _INTERNAL_PBDRV_BLUETOOTH_NXT_H_

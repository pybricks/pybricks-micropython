// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Derived static random Bluetooth address, shared by all Bluetooth drivers.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_ADDRESS_H_
#define _INTERNAL_PBDRV_BLUETOOTH_ADDRESS_H_

#include <stddef.h>
#include <stdint.h>

#include "pbio_version_hash.h"

/**
 * Derives a static random Bluetooth address from the chip's fixed address
 * and the firmware version hash.
 *
 * The result is unique per hub and stable across reboots, so host-side
 * bonding stays valid. It changes with each new firmware version, so hosts
 * don't reuse stale cached data after a firmware update.
 *
 * @param [out] addr     The derived address, 6 bytes in little-endian order.
 * @param [in]  bd_addr  The chip's fixed 6-byte address, used as seed.
 */
static inline void pbdrv_bluetooth_derive_static_address(uint8_t *addr, const uint8_t *bd_addr) {
    // FNV-1a 64-bit over the chip address and the firmware version hash.
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < 6; i++) {
        hash = (hash ^ bd_addr[i]) * 0x00000100000001b3;
    }
    for (const char *c = PBIO_VERSION_HASH; *c; c++) {
        hash = (hash ^ (uint8_t)*c) * 0x00000100000001b3;
    }
    for (size_t i = 0; i < 6; i++) {
        addr[i] = hash >> (i * 8);
    }
    // The two most significant bits must be 1 for a static random address.
    addr[5] |= 0xC0;
}

#endif // _INTERNAL_PBDRV_BLUETOOTH_ADDRESS_H_

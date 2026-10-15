// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors


#ifndef _PBDRV_HARDWARE_H_
#define _PBDRV_HARDWARE_H_

#include <stdint.h>

#include <pbdrv/config.h>

/**
 * Size of the hub MAC address in bytes.
 */
#define PBDRV_HARDWARE_MAC_ADDRESS_SIZE (6)

#if PBDRV_CONFIG_HARDWARE

/**
 * Gets a user facing description of the hub hardware version.
 *
 * Hardware detection may rely on peripherals that are not up yet, so the
 * result can change during boot. It is not meant to be cached.
 *
 * @return  The hardware version, or @c NULL if it is not known.
 */
const char *pbdrv_hardware_get_version(void);

/**
 * Gets the globally unique MAC address that the manufacturer registered for
 * this hub and programmed into it at the factory.
 *
 * This is what the hub is expected to present as its Bluetooth Classic
 * address, and what it uses as its USB serial number where it has one.
 *
 * Where it is read from depends on the hub, so it is not necessarily known
 * from the start of boot. Callers must call this when they know it is
 * available or retry.
 *
 * @return  ::PBDRV_HARDWARE_MAC_ADDRESS_SIZE bytes, most significant first, or
 *          @c NULL if the address is not known, either not yet or not on this
 *          platform at all.
 */
const uint8_t *pbdrv_hardware_get_mac_address(void);

/**
 * Records the MAC address of this hub, to be handed out by
 * ::pbdrv_hardware_get_mac_address from here on.
 *
 * Called by whichever driver is able to read it on this platform, as soon as
 * it has it. Until then the address reads as not known.
 *
 * @param [in] address  ::PBDRV_HARDWARE_MAC_ADDRESS_SIZE bytes, most
 *                      significant first. Copied, so it need not outlive the
 *                      call.
 */
void pbdrv_hardware_set_mac_address(const uint8_t *address);

#else // PBDRV_CONFIG_HARDWARE

static inline const char *pbdrv_hardware_get_version(void) {
    return NULL;
}

static inline const uint8_t *pbdrv_hardware_get_mac_address(void) {
    return NULL;
}

static inline void pbdrv_hardware_set_mac_address(const uint8_t *address) {
}

#endif // PBDRV_CONFIG_HARDWARE

#endif // _PBDRV_HARDWARE_H_

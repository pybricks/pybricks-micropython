// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_HARDWARE_STM32

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/hardware.h>

const char *pbdrv_hardware_get_version(void) {
    // Nothing consumes this on these hubs yet, and it is not known where the
    // hardware version is stored, if anywhere.
    return NULL;
}

static uint8_t mac_address[PBDRV_HARDWARE_MAC_ADDRESS_SIZE];
static bool mac_address_known;

const uint8_t *pbdrv_hardware_get_mac_address(void) {
    return mac_address_known ? mac_address : NULL;
}

// TODO, find where this is set.
void pbdrv_hardware_set_mac_address(const uint8_t *address) {
    memcpy(mac_address, address, sizeof(mac_address));
    mac_address_known = true;
}



#endif // PBDRV_CONFIG_HARDWARE_STM32

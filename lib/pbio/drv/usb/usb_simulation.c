// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_SIMULATION

#include <pbdrv/usb.h>

#include <pbio/error.h>
#include <pbio/os.h>

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

void pbdrv_usb_init_device(void) {
}

#endif // PBDRV_CONFIG_USB_SIMULATION

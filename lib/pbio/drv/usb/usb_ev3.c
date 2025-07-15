// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 / TI AM1808 / Mentor Graphics MUSBMHDRC driver
// implementing a bespoke USB stack for Pybricks USB protocol

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_EV3

#include <stdint.h>

#include <pbdrv/usb.h>
#include <pbio/util.h>

#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_usbOtg_AM1808.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_usbphyGS60.h>
#include <tiam1808/hw/hw_usb.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_psc_AM1808.h>
#include <tiam1808/psc.h>

#include <pbdrv/clock.h>


void pbdrv_usb_init(void) {
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    // This function is not used on EV3
    return PBDRV_USB_BCD_NONE;
}

// TODO: Reimplement the following functions as appropriate
// (mphalport.c and the "host" layer are currently being refactored)
uint32_t pbdrv_usb_write(const uint8_t *data, uint32_t size) {
    // Return the size requested so that caller doesn't block.
    return size;
}

uint32_t pbdrv_usb_rx_data_available(void) {
    return 0;
}

int32_t pbdrv_usb_get_char(void) {
    return -1;
}

void pbdrv_usb_tx_flush(void) {
}

bool pbdrv_usb_connection_is_active(void) {
    return false;
}

#endif // PBDRV_CONFIG_USB_EV3

// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Main file for EV3 USB driver.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_EV3

#include <stdbool.h>

#include <contiki.h>

#include <pbdrv/usb.h>
#include <pbio/util.h>

PROCESS(pbdrv_usb_process, "USB");

static bool usb_connected;

void pbdrv_usb_init(void) {

    process_start(&pbdrv_usb_process);

    process_poll(&pbdrv_usb_process);
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return usb_connected ? PBDRV_USB_BCD_STANDARD_DOWNSTREAM : PBDRV_USB_BCD_NONE;
}

PROCESS_THREAD(pbdrv_usb_process, ev, data) {

    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_USB_EV3

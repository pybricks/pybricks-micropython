// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Main file for STM32F4 USB driver.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_STM32F4

#include <contiki.h>
#include <stm32f4xx_hal.h>
#include <usbd_core.h>

#include "./usb_stm32.h"

PROCESS(pbdrv_usb_process, "USB");

static USBD_HandleTypeDef husbd;
static PCD_HandleTypeDef hpcd;

// Device-specific USB driver implementation.

/**
 * Callback for connecting USB OTG FS interrupt in platform.c.
 */
void pbdrv_usb_stm32_handle_otg_fs_irq(void) {
    HAL_PCD_IRQHandler(&hpcd);
}

// Common USB driver implementation.

void pbdrv_usb_init(void) {
    // Link the driver data structures
    husbd.pData = &hpcd;
    hpcd.pData = &husbd;

    USBD_Init(&husbd, NULL, 0);
    process_start(&pbdrv_usb_process);
}

// Event loop

PROCESS_THREAD(pbdrv_usb_process, ev, data) {
    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_USB_STM32F4

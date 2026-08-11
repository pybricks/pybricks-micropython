// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup UsbDriver Driver: USB
 * @{
 */

#ifndef _PBDRV_USB_H_
#define _PBDRV_USB_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#if PBDRV_CONFIG_USB

/**
 * Initializes the USB process on boot.
 */
void pbio_usb_init(void);

/**
 * De-initializes the USB process on soft-poweroff. Keeps charging if supported.
 */
void pbio_usb_deinit(void);

/**
 * Indicates if a Pybricks app is connected and configured.
 *
 * @retval  true if active, so the host has subscribed to events.
 */
bool pbio_usb_connection_is_active(void);

#else // PBDRV_CONFIG_USB

static inline void pbio_usb_init(void) {
}

static inline void pbio_usb_deinit(void) {
}

static inline bool pbio_usb_connection_is_active(void) {
    return false;
}

#endif // PBDRV_CONFIG_USB

#endif // _PBDRV_USB_H_

/** @} */

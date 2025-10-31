// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common interface shared by USB drivers

#ifndef _INTERNAL_PBDRV_USB_H_
#define _INTERNAL_PBDRV_USB_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

/**
 * Initializes the USB driver on boot.
 */
void pbdrv_usb_init(void);

/**
 * De-initializes the USB driver for data transfers on soft-poweroff. Keeps charging if supported.
 */
void pbdrv_usb_deinit(void);

#else // PBDRV_CONFIG_USB

#define pbdrv_usb_init()
#define pbdrv_usb_deinit()

#endif // PBDRV_CONFIG_USB

#endif // _INTERNAL_PBDRV_USB_H_

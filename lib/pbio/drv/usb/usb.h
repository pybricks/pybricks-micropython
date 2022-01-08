// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common interface shared by USB drivers

#ifndef _INTERNAL_PBDRV_USB_H_
#define _INTERNAL_PBDRV_USB_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

/**
 * Initializes the USB driver.
 */
void pbdrv_usb_init(void);

#else // PBDRV_CONFIG_USB

#define pbdrv_usb_init()

#endif // PBDRV_CONFIG_USB

#endif // _INTERNAL_PBDRV_USB_H_

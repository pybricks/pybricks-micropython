// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup UsbDriver Driver: USB
 * @{
 */

#ifndef _PBDRV_USB_H_
#define _PBDRV_USB_H_

#include <stdbool.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * Indicates battery charging capabilites that were detected on a USB port.
 */
typedef enum {
    /** The USB cable is not connected (no VBUS). */
    PBDRV_USB_BCD_NONE,
    /** The USB cable is connected to a non-standard charger or PS/2 port. */
    PBDRV_USB_BCD_NONSTANDARD,
    /** The USB cable is connected to standard downstream port. */
    PBDRV_USB_BCD_STANDARD_DOWNSTREAM,
    /** The USB cable is connected to charging downstream port. */
    PBDRV_USB_BCD_CHARGING_DOWNSTREAM,
    /** The USB cable is connected to dedicated charging port. */
    PBDRV_USB_BCD_DEDICATED_CHARGING,
} pbdrv_usb_bcd_t;

#if PBDRV_CONFIG_USB

/**
 * Gets the result of the USB battery charger detection.
 * @return              The result.
 */
pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void);

/**
 * Transmits the given buffer over the USB stdout stream.
 * @return              The result of the operation.
 */
pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size);

/**
 * Indicates if the USB stdout stream is idle.
 * @return              true if the USB stdout stream is idle.
*/
bool pbdrv_usb_stdout_tx_is_idle(void);

#else // PBDRV_CONFIG_USB

static inline pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

static inline pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size) {
    return PBIO_SUCCESS;
}

static inline bool pbdrv_usb_stdout_tx_is_idle(void) {
    return true;
}

#endif // PBDRV_CONFIG_USB

#endif // _PBDRV_USB_H_

/** @} */

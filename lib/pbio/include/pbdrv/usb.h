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

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * Indicates battery charging capabilites that were detected on a USB port.
 */
typedef enum {
    // NOTE: These values are part of the MicroPython API, don't change the numbers.

    /** The USB cable is not connected (no VBUS). */
    PBDRV_USB_BCD_NONE = 0,
    /** The USB cable is connected to a non-standard charger or PS/2 port. */
    PBDRV_USB_BCD_NONSTANDARD = 1,
    /** The USB cable is connected to standard downstream port. */
    PBDRV_USB_BCD_STANDARD_DOWNSTREAM = 2,
    /** The USB cable is connected to charging downstream port. */
    PBDRV_USB_BCD_CHARGING_DOWNSTREAM = 3,
    /** The USB cable is connected to dedicated charging port. */
    PBDRV_USB_BCD_DEDICATED_CHARGING = 4,
} pbdrv_usb_bcd_t;

#if PBDRV_CONFIG_USB

// producer
uint32_t pbdrv_usb_rx_get_free(void);
void pbdrv_usb_rx_write(const uint8_t *data, uint32_t size);
// consumer
typedef bool (*pbdrv_usb_rx_callback_t)(uint8_t c);
void pbdrv_usb_rx_set_callback(pbdrv_usb_rx_callback_t callback);
uint32_t pbdrv_usb_rx_get_available(void);
pbio_error_t pbdrv_usb_rx(uint8_t *data, uint32_t *size);

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

#define pbdrv_usb_rx_get_free() 0
#define pbdrv_usb_rx_write(data, size)
#define pbdrv_usb_rx_set_callback(callback)
#define pbdrv_usb_rx_get_available() 0
#define pbdrv_usb_rx(data, size) PBIO_SUCCESS
#define pbdrv_usb_get_bcd() PBDRV_USB_BCD_NONE
#define pbdrv_usb_stdout_tx(data, size) PBIO_SUCCESS
#define pbdrv_usb_stdout_tx_is_idle() true

#endif // PBDRV_CONFIG_USB

#endif // _PBDRV_USB_H_

/** @} */

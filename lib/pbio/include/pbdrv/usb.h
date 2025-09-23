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
#include <pbio/protocol.h>

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

/**
 * Callback that is called when receiving a Pybricks command.
 *
 * @param [in]  data        The data that was received.
 * @param [in]  size        The size of @p data in bytes.
 */
typedef pbio_pybricks_error_t (*pbdrv_usb_receive_handler_t)(const uint8_t *data, uint32_t size);

#if PBDRV_CONFIG_USB

/**
 * Gets the result of the USB battery charger detection.
 * @return              The result.
 */
pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void);

/**
 * Registers a callback that will be called when Pybricks data is received.
 *
 * @param [in]  handler     The function that will be called.
 */
void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler);

/**
 * Schedules Pybricks status to be sent soon.
 *
 * The data length is always ::PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE.
 */
void pbdrv_usb_schedule_status_update(const uint8_t *status_msg);

/**
 * Transmits the given buffer over the USB stdout stream.
 * @return              The result of the operation.
 */
pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size);

/**
 * Gets the number of bytes that can be queued for sending stdout via USB.
 *
 * Returns UINT32_MAX if there is no USB connection or no app is subscribed to
 * stdout.
 *
 * @return              The number of bytes that can be queued.
 */
uint32_t pbdrv_usb_stdout_tx_available(void);

/**
 * Indicates if the USB stdout stream is idle.
 * @return              true if the USB stdout stream is idle.
*/
bool pbdrv_usb_stdout_tx_is_idle(void);

/**
 * Indicates if a Pybricks app is connected and configured.
 *
 * @retval  true if active, so the host has subscribed to events.
 */
bool pbdrv_usb_connection_is_active(void);

#else // PBDRV_CONFIG_USB

static inline pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

static inline void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler) {
}

static inline void pbdrv_usb_schedule_status_update(const uint8_t *status_msg) {
}

static inline pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size) {
    return PBIO_SUCCESS;
}

static inline uint32_t pbdrv_usb_stdout_tx_available(void) {
    return UINT32_MAX;
}

static inline bool pbdrv_usb_stdout_tx_is_idle(void) {
    return true;
}

static inline bool pbdrv_usb_connection_is_active(void) {
    return false;
}

#endif // PBDRV_CONFIG_USB

#endif // _PBDRV_USB_H_

/** @} */

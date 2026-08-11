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
 * Sets a callback to be called when a USB host is connected or disconnected.
 *
 * @param [in]  callback    The function that will be called.
 */
void pbdrv_usb_set_host_connection_changed_callback(pbio_util_void_callback_t callback);

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

/**
 * Sends a value notification and await it.
 *
 * Uses the same mechanism as stdout or status events, but is user-awaitable.
 *
 * This does not not use a ringbuffer. Await operation before sending more.
 *
 * @param [in] state    Protothread state.
 * @param [in] event    Event type (status, stdout, or app data, etc.)
 * @param [in] data     Data to send.
 * @param [in] size     Data size, not counting event type byte.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_INVALID_OP if there is no connection.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 *                      ::PBIO_ERROR_BUSY if this operation is already ongoing.
 *                      ::PBIO_ERROR_INVALID_ARG if @p size is too large.
 */
pbio_error_t pbdrv_usb_send_event_notification(pbio_os_state_t *state, pbio_pybricks_event_t event, const uint8_t *data, size_t size);

/**
 * Stores a string in the USB stdout ring buffer.
 *
 * @param data      The string data.
 * @param len       The length of the string data.
 */
void pbdrv_usb_debug_print(const char *data, size_t len);

#else // PBDRV_CONFIG_USB

static inline pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

static inline void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler) {
}

static inline void pbdrv_usb_set_host_connection_changed_callback(pbio_util_void_callback_t callback) {
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

static inline pbio_error_t pbdrv_usb_send_event_notification(pbio_os_state_t *state, pbio_pybricks_event_t event, const uint8_t *data, size_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_usb_debug_print(const char *data, size_t len) {
}

#endif // PBDRV_CONFIG_USB

#endif // _PBDRV_USB_H_

/** @} */

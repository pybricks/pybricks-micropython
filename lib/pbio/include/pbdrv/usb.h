// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common interface shared by USB drivers

#ifndef _INTERNAL_PBDRV_USB_H_
#define _INTERNAL_PBDRV_USB_H_

#include <pbdrv/config.h>
#include <pbio/os.h>

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

#include <pbio/cobs.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#include <stdint.h>

#define PBDRV_USB_TRANSMIT_TIMEOUT (500)

/**
 * Platform specific device initialization.
 */
void pbdrv_usb_init(void);

/**
 * Platform specific device deinitialization.
 */
void pbdrv_usb_deinit(void);

/**
 * Gets bytes most recently received on the data OUT endpoint and copies them
 * to the provided buffer.
 *
 * The driver's receive buffer is then cleared and prepared to receive again.
 *
 * The host to hub direction is a raw byte stream (the message framing is
 * handled by the common driver), so the returned bytes are an arbitrary slice
 * of that stream, not necessarily a whole message.
 *
 * @param [in] data     Buffer to copy the bytes to.
 * @return              Number of bytes copied. Zero means nothing was available.
 */
uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data);

/**
 * Sends and awaits an arbitrarily sized message on the data IN endpoint.
 *
 * Driver-specific implementation. Must return within ::PBDRV_USB_TRANSMIT_TIMEOUT.
 *
 * The USB process ensures that only one call is made at once.
 *
 * @param [in] state    Protothread state.
 * @param [in] data     Data to send.
 * @param [in] size     Data size.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 *                      ::PBIO_ERROR_BUSY if this operation is already ongoing.
 *                      ::PBIO_ERROR_TIMEDOUT if the operation was started but could not complete.
 */
pbio_error_t pbdrv_usb_tx_message(pbio_os_state_t *state, const uint8_t *data, uint32_t size);

/**
 * Notifies the common driver that the host's serial control line state (DTR)
 * changed. Called by the platform driver, typically from interrupt context.
 *
 * DTR asserted means a host application has opened the serial port and is the
 * USB analog of a BLE host subscribing to notifications.
 *
 * @param [in] dtr      True if DTR is asserted (port open), otherwise false.
 */
void pbdrv_usb_on_dtr_changed(bool dtr);

/**
 * Resets the driver transmission state.
 *
 * @param [in] state    Protothread state.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 */
pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state);

/**
 * Waits for USB to be plugged. Detects what charger type is connected if
 * applicable.
 *
 * @param [in] state    Protothread state.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 */
pbio_error_t pbdrv_usb_wait_until_configured(pbio_os_state_t *state);

/**
 * Tests if USB is ready for communication.
 */
bool pbdrv_usb_is_ready(void);

/**
 * Gets the result of the USB battery charger detection.
 * @return              The result.
 */
pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void);

#else // PBDRV_CONFIG_USB

static inline void pbdrv_usb_init(void) {
}

static inline void pbdrv_usb_deinit(void) {
}

static inline uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {
    return 0;
}

static inline pbio_error_t pbdrv_usb_tx_message(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_usb_wait_until_configured(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline bool pbdrv_usb_is_ready(void) {
    return false;
}

static inline pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

#endif // PBDRV_CONFIG_USB

#endif // _INTERNAL_PBDRV_USB_H_

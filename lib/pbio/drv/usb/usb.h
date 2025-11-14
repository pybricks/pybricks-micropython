// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common interface shared by USB drivers

#ifndef _INTERNAL_PBDRV_USB_H_
#define _INTERNAL_PBDRV_USB_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#include <stdint.h>

#define PBDRV_USB_TRANSMIT_TIMEOUT (50)

/**
 * Initializes the USB driver on boot.
 */
void pbdrv_usb_init(void);

/**
 * Platform specific device initialization.
 */
void pbdrv_usb_init_device(void);

/**
 * De-initializes the USB driver for data transfers on soft-poweroff. Keeps charging if supported.
 */
void pbdrv_usb_deinit(void);

/**
 * Platform specific device deinitialization.
 */
void pbdrv_usb_deinit_device(void);

/**
 * Gets most recent incoming message and copies it to provided buffer.
 *
 * The message is then cleared and the driver prepares to read a new message.
 *
 * @param [in] data     Buffer to copy the message to.
 * @return              Number of bytes copied. Zero means nothing was available.
 */
uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data);

/**
 * Sends and awaits event message from hub to host via the Pybricks USB interface OUT endpoint.
 *
 * Driver-specific implementation. Must return within ::PBDRV_USB_TRANSMIT_TIMEOUT.
 *
 * The USB process ensures that only one call is made at once.
 *
 * Data must include the endpoint type and event code, so size is at least 2.
 *
 * @param [in] state    Protothread state.
 * @param [in] data     Data to send.
 * @param [in] size     Data size.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_INVALID_OP if there is no connection.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 *                      ::PBIO_ERROR_BUSY if this operation is already ongoing.
 *                      ::PBIO_ERROR_INVALID_ARG if @p size is too large.
 *                      ::PBIO_ERROR_TIMEDOUT if the operation was started but could not complete.
 */
pbio_error_t pbdrv_usb_tx_event(pbio_os_state_t *state, const uint8_t *data, uint32_t size);

/**
 * Sends and awaits response to an earlier incoming message.
 *
 * Driver-specific implementation. Must return within ::PBDRV_USB_TRANSMIT_TIMEOUT.
 *
 * The USB process ensures that only one call is made at once.
 *
 * @param [in] state    Protothread state.
 * @param [in] code     Error code to send.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_INVALID_OP if there is no connection.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 *                      ::PBIO_ERROR_BUSY if this operation is already ongoing.
 *                      ::PBIO_ERROR_TIMEDOUT if the operation was started but could not complete.
 */
pbio_error_t pbdrv_usb_tx_response(pbio_os_state_t *state, pbio_pybricks_error_t code);

/**
 * Resets the driver transmission state.
 *
 * @param [in] state    Protothread state.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 */
pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state);

/**
 * Waits for USB to be plugged in and detects what charger type is connected.
 *
 * @param [in] state    Protothread state.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 *                      ::PBIO_ERROR_NOT_SUPPORTED if platform has no charger.
 */
pbio_error_t pbdrv_usb_wait_for_charger(pbio_os_state_t *state);

/**
 * Tests if USB is ready for communication.
 */
bool pbdrv_usb_is_ready(void);

#else // PBDRV_CONFIG_USB

static inline void pbdrv_usb_init(void) {
}

static inline void pbdrv_usb_deinit(void) {
}

static inline void pbdrv_usb_init_device(void) {
}

static inline void pbdrv_usb_deinit_device(void) {
}

static inline pbio_error_t pbdrv_usb_tx(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {
    return 0;
}

static inline pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline pbio_error_t pbdrv_usb_wait_for_charger(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline bool pbdrv_usb_is_ready(void) {
    return false;
}


#endif // PBDRV_CONFIG_USB

#endif // _INTERNAL_PBDRV_USB_H_

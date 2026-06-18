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

#define PBDRV_USB_TRANSMIT_TIMEOUT (500)

/** Frame delimiter byte. */
#define PBDRV_USB_COBS_DELIMITER 0x00

/**
 * Maximum size of a host notification, and of any other host-facing message.
 *
 * This is deliberately decoupled from the USB hardware packet size: it bounds
 * how large a single message can be, independent of how the transport splits
 * the resulting byte stream into hardware packets.
 *
 * REVISIT: this is a system-level concept that should eventually live in pbsys
 * config and be shared with the BLE transport.
 */
#define PBSYS_CONFIG_HOST_NOTIFICATION_SIZE (62)

/** Maximum size of a decoded message (message type byte plus payload). */
#define PBDRV_USB_MAX_DECODED_MESSAGE_SIZE PBSYS_CONFIG_HOST_NOTIFICATION_SIZE

/** Maximum size of a COBS-encoded frame including the trailing delimiter. */
#define PBDRV_USB_MAX_ENCODED_MESSAGE_SIZE (PBDRV_USB_MAX_DECODED_MESSAGE_SIZE + PBDRV_USB_MAX_DECODED_MESSAGE_SIZE / 254 + 2)

/**
 * Largest single USB packet any platform delivers on the data OUT endpoint
 * (EV3 high-speed bulk). The common driver provides a receive scratch buffer of
 * this size; the framing layer then reassembles messages from the raw byte
 * stream, which may pack several small frames into one hardware packet.
 */
#define PBDRV_USB_RX_PACKET_MAX_SIZE (512)

/**
 * COBS-encodes @p len bytes from @p src into @p dst and appends the frame
 * delimiter. @p dst must have room for at least ::PBDRV_USB_MAX_ENCODED_MESSAGE_SIZE bytes
 * when @p len is at most ::PBDRV_USB_MAX_DECODED_MESSAGE_SIZE.
 *
 * @return  Number of bytes written to @p dst, including the trailing delimiter.
 */
uint32_t pbdrv_usb_cobs_encode(const uint8_t *src, uint32_t len, uint8_t *dst);

/**
 * COBS-decodes @p len bytes from @p src (a single frame with the delimiter
 * already stripped) into @p dst.
 *
 * @return  Number of decoded bytes, or 0 if the frame was empty or malformed.
 */
uint32_t pbdrv_usb_cobs_decode(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t dst_max);

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
 * Gets bytes most recently received on the data OUT endpoint and copies them
 * to the provided buffer.
 *
 * The driver's receive buffer is then cleared and prepared to receive again.
 *
 * The host to hub direction is a raw byte stream (the message framing is
 * handled by the common driver), so the returned bytes are an arbitrary slice
 * of that stream, not necessarily a whole message.
 *
 * @param [in] data     Buffer to copy the bytes to. Must be at least
 *                      ::PBDRV_USB_RX_PACKET_MAX_SIZE bytes.
 * @return              Number of bytes copied. Zero means nothing was available.
 */
uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data);

/**
 * Sends and awaits a raw chunk of bytes on the data IN endpoint.
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
pbio_error_t pbdrv_usb_tx_chunk(pbio_os_state_t *state, const uint8_t *data, uint32_t size);

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

static inline pbio_error_t pbdrv_usb_wait_until_configured(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline bool pbdrv_usb_is_ready(void) {
    return false;
}


#endif // PBDRV_CONFIG_USB

#endif // _INTERNAL_PBDRV_USB_H_

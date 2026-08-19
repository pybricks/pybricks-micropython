// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_USB

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pbdrv/usb.h>

#include <pbdrv/config.h>
#include <pbio/usb.h>

#include <pbio/cobs.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#include <string.h>

#include <pbio/bluetooth.h>

#include <pbio/version.h>

#include <pbsys/config.h>
#include <pbsys/storage.h>
#include <pbsys/command.h>

#include <pbsys/host.h>
#include <pbsys/status.h>

// Host event size (already includes its 1 event byte) + 1 EP type byte.
#define PBSYS_USB_MAX_DECODED_MESSAGE_SIZE (PBSYS_CONFIG_HOST_EVENT_OUT_SIZE + 1)
#define PBSYS_USB_MAX_ENCODED_PACKET_SIZE (PBIO_COBS_ENCODED_BUFFER_SIZE(PBSYS_USB_MAX_DECODED_MESSAGE_SIZE))

/**
 * Whether the host has opened the serial port (DTR asserted). This detects
 * disconnection gracefully, even if the host abruptly goes away (e.g. the
 * browser tab is closed) without unsubscribing.
 */
static bool pbio_usb_dtr;

/**
 * Whether the host is subscribed to our outgoing event messages. This is the
 * USB analog of a BLE host subscribing to notifications.
 *
 * Unlike DTR, this is asserted explicitly by the host application rather than
 * automatically by the OS when the port is opened. It gates the initial event
 * flood so that we don't bombard the host with events the moment DTR is
 * (possibly automatically) asserted.
 */
static bool pbio_usb_subscribed;

bool pbio_usb_connection_is_active(void) {
    return pbdrv_usb_is_ready() && pbio_usb_dtr && pbio_usb_subscribed;
}

static uint32_t pbio_usb_copy_str(uint8_t *buf, const char *str) {
    uint32_t size = strlen(str);

    // Data with 4 byte header must fit in the unencoded frame.
    if (size > PBSYS_USB_MAX_DECODED_MESSAGE_SIZE - 4) {
        size = PBSYS_USB_MAX_DECODED_MESSAGE_SIZE - 4;
    }
    memcpy(buf, str, size);
    return size;
}

/**
 * Provides characteristic values when a USB host requests a read. This is the
 * USB analog of a BLE host reading the device info or Pybricks characteristics.
 *
 * REVISIT: this reaches up into pbsys for the value sources, much like each BLE
 * driver still does today. We should invert this so pbsys registers a single read
 * handler shared by USB and all BLE drivers.
 */
static uint32_t pbio_usb_read_characteristic(uint8_t service, uint16_t char_id, uint8_t *buf) {
    switch (service) {
        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_GATT:
            switch (char_id) {
                case PBIO_GATT_DEVICE_NAME_CHAR_UUID:
                    return pbio_usb_copy_str(buf, pbdrv_bluetooth_get_hub_name());
                case PBIO_GATT_FIRMWARE_VERSION_CHAR_UUID:
                    return pbio_usb_copy_str(buf, PBIO_VERSION_STR);
                case PBIO_GATT_SOFTWARE_VERSION_CHAR_UUID:
                    return pbio_usb_copy_str(buf, PBIO_PROTOCOL_VERSION_STR);
                case PBIO_GATT_PNP_ID_CHAR_UUID:
                    pbio_pybricks_pnp_id(buf, PBDRV_CONFIG_HUB_KIND, PBDRV_CONFIG_HUB_VARIANT);
                    return PBIO_PYBRICKS_PNP_ID_SIZE;
                default:
                    return 0;
            }
        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_PYBRICKS:
            switch (char_id) {
                case 0x0003: // Hub capabilities
                    pbio_pybricks_hub_capabilities(buf,
                        PBSYS_USB_MAX_DECODED_MESSAGE_SIZE - 2,
                        PBSYS_CONFIG_APP_FEATURE_FLAGS,
                        pbsys_storage_get_maximum_program_size(),
                        PBSYS_CONFIG_HMI_NUM_SLOTS);
                    return PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE;
                default:
                    return 0;
            }
        default:
            return 0;
    }
}

/**
 * Incoming COBS frame assembly buffer (encoded bytes, delimiter excluded).
 */
static uint8_t pbio_usb_rx_frame[PBSYS_USB_MAX_ENCODED_PACKET_SIZE];
static uint32_t pbio_usb_rx_frame_len;
static bool pbio_usb_rx_overflow;

void pbio_usb_on_dtr_changed(bool dtr) {
    if (dtr == pbio_usb_dtr) {
        return;
    }

    pbio_usb_dtr = dtr;

    // Drop any partially-assembled incoming frame. Otherwise stray bytes from a
    // previous port opener (e.g. an OS modem probe like ModemManager, which
    // writes AT strings with no frame delimiter) would prepend to and corrupt
    // the first real frame of this new connection.
    pbio_usb_rx_frame_len = 0;
    pbio_usb_rx_overflow = false;

    if (!dtr) {
        // Host closed the port. The subscription implicitly falls with DTR. In
        // practice the host rarely unsubscribes explicitly; it just lets DTR
        // drop, which we detect even if the host abruptly goes away.
        pbio_usb_subscribed = false;
    }

    pbsys_host_connection_changed();

    pbio_os_request_poll();
}

/**
 * Sets whether the host is subscribed to event notifications and notifies
 * listeners of the connection state change.
 */
static void pbio_usb_set_subscribed(bool subscribed) {
    if (subscribed == pbio_usb_subscribed) {
        return;
    }

    pbio_usb_subscribed = subscribed;

    if (subscribed) {
        // Host just subscribed. Send the current status right away, like the
        // first notification after a BLE host subscribes. Device info is not
        // pushed; the host reads it on demand via read requests.
        pbsys_status_update_emit();
    }

    pbsys_host_connection_changed();

    pbio_os_request_poll();
}

/**
 * Pending command response to the most recently received command.
 *
 * The host keeps a single command outstanding at a time (like a BLE write with
 * response), so a single command response slot is sufficient. The buffer holds
 * the message payload `[tag, status32]`; the RESPONSE message type is added as
 * the COBS prefix at transmit time. `tag` echoes the byte from the command
 * that produced this response so the host can correlate them.
 */
static uint8_t pbio_usb_command_response_buf[sizeof(uint8_t) + sizeof(uint32_t) + 1];
static bool pbio_usb_command_response_pending;

/**
 * Pending reply to the most recently received read request.
 *
 * Like responses, the host keeps a single read outstanding at a time, so a
 * single reply slot is sufficient. The buffer holds the message payload
 * `[service, char_id_lo, char_id_hi, value...]`; the READ_REPLY message type
 * is added as the COBS prefix at transmit time.
 */
static uint8_t pbio_usb_read_reply_buf[PBSYS_USB_MAX_DECODED_MESSAGE_SIZE];
static uint32_t pbio_usb_read_reply_len;
static bool pbio_usb_read_reply_pending;

/** Number of header bytes before the value in a read reply. */
#define pbio_usb_READ_REPLY_HEADER_SIZE 3

/**
 * Non-blocking poll handler to process incoming bytes.
 *
 * Bytes are assembled into COBS frames. Each completed frame is decoded into a
 * host-to-hub message whose first byte selects the type: a command is handled
 * synchronously and its result queued as a response, while a read request is
 * answered synchronously and queued as a read reply. This never depends on the
 * transmit state, so it cannot deadlock.
 */
static void pbio_usb_handle_data_in(void) {

    // Bytes are copied here so the driver can immediately queue the next
    // receive. Only the single USB process thread runs this, so a static
    // scratch buffer is safe and keeps the worst-case packet off the stack.
    static uint8_t data_in[PBSYS_USB_MAX_ENCODED_PACKET_SIZE];
    static uint8_t msg[PBSYS_USB_MAX_DECODED_MESSAGE_SIZE];
    uint32_t size = pbdrv_usb_get_data_and_start_receive(data_in);

    for (uint32_t i = 0; i < size; i++) {
        uint8_t byte = data_in[i];

        if (byte != PBIO_COBS_DELIMITER) {
            if (pbio_usb_rx_frame_len < sizeof(pbio_usb_rx_frame)) {
                pbio_usb_rx_frame[pbio_usb_rx_frame_len++] = byte;
            } else {
                // Frame too big. Discard until the next delimiter resyncs us.
                pbio_usb_rx_overflow = true;
            }
            continue;
        }

        // Delimiter reached: end of frame.
        if (!pbio_usb_rx_overflow && pbio_usb_rx_frame_len > 0) {
            uint8_t msg_type;
            uint32_t msg_size = pbio_cobs_decode_prefixed(
                pbio_usb_rx_frame, pbio_usb_rx_frame_len, &msg_type, msg, sizeof(msg));

            // The decoded prefix is the host-to-hub message type and the rest
            // is its payload.
            if (msg_size >= 1 && msg_type == PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE) {
                // Subscribe or unsubscribe to event notifications. The payload
                // is a single byte: 1 to subscribe, 0 to unsubscribe.
                pbio_usb_set_subscribed(msg[0]);
            } else if (msg_size >= 2 && msg_type == PBIO_PYBRICKS_OUT_EP_MSG_COMMAND) {
                // The command payload is [tag, ...payload]. The tag is opaque
                // to us: echo it back in the response so the host can correlate
                // a late response with the command that produced it. The
                // payload after the tag is the same as a BLE command write.
                pbio_usb_command_response_buf[0] = msg[0];
                pbio_set_uint32_le(&pbio_usb_command_response_buf[1],
                    pbsys_handle_command(&msg[1], msg_size - 1));
                pbio_usb_command_response_pending = true;
                pbio_os_request_poll();
            } else if (msg_size >= 3 && msg_type == PBIO_PYBRICKS_OUT_EP_MSG_READ) {
                // A read request payload is [service, char_id_lo, char_id_hi].
                // Read the value synchronously and queue the reply, echoing the
                // selector so the host can correlate it.
                uint8_t service = msg[0];
                uint16_t char_id = pbio_get_uint16_le(&msg[1]);
                pbio_usb_read_reply_buf[0] = service;
                pbio_usb_read_reply_buf[1] = msg[1];
                pbio_usb_read_reply_buf[2] = msg[2];
                uint32_t value_size = pbio_usb_read_characteristic(
                    service, char_id, &pbio_usb_read_reply_buf[pbio_usb_READ_REPLY_HEADER_SIZE]);
                pbio_usb_read_reply_len = pbio_usb_READ_REPLY_HEADER_SIZE + value_size;
                pbio_usb_read_reply_pending = true;
                pbio_os_request_poll();
            }
        }

        pbio_usb_rx_frame_len = 0;
        pbio_usb_rx_overflow = false;
    }
}

static void pbio_usb_reset_state(void) {
    pbio_usb_on_dtr_changed(false);
    pbio_usb_subscribed = false;
    pbio_usb_command_response_pending = false;
    pbio_usb_read_reply_pending = false;
    pbio_usb_rx_frame_len = 0;
    pbio_usb_rx_overflow = false;
}

static pbio_os_process_t pbio_usb_process;

static pbio_error_t pbio_usb_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    static uint32_t *event_size;
    static uint8_t *event_buf;

    // COBS-encoded frame scratch buffer, populated just before each transmit.
    static uint8_t tx_frame[PBSYS_USB_MAX_ENCODED_PACKET_SIZE];
    static uint32_t tx_frame_len;

    pbio_error_t err;

    // Runs every time. If there is no connection, there just won't be data.
    pbio_usb_handle_data_in();

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        pbsys_host_connection_changed();

        // Run charger detection: wait for USB to become physically plugged in.
        PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_wait_until_configured(&sub));

        while (pbio_usb_process.request != PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL && pbdrv_usb_is_ready()) {

            // Find out what we should send, if anything, prioritizing replies
            // to host requests (command response, then read reply), then status, then
            // stdout, then other events. Unlike events, replies do not require
            // an active connection, since they answer a request that was just
            // received.
            if (pbio_usb_command_response_pending) {
                tx_frame_len = pbio_cobs_encode_prefixed(PBIO_PYBRICKS_IN_EP_MSG_RESPONSE,
                    pbio_usb_command_response_buf, sizeof(pbio_usb_command_response_buf) - 1, tx_frame);

                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_message(&sub, tx_frame, tx_frame_len));
                pbio_usb_command_response_pending = false;
                if (err != PBIO_SUCCESS) {
                    pbio_usb_reset_state();
                    PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
                }
            } else if (pbio_usb_read_reply_pending) {
                // Reply to a characteristic read request.
                tx_frame_len = pbio_cobs_encode_prefixed(PBIO_PYBRICKS_IN_EP_MSG_READ_REPLY,
                    pbio_usb_read_reply_buf, pbio_usb_read_reply_len, tx_frame);

                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_message(&sub, tx_frame, tx_frame_len));
                pbio_usb_read_reply_pending = false;
                if (err != PBIO_SUCCESS) {
                    pbio_usb_reset_state();
                    PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
                }
            } else if (pbio_usb_connection_is_active() && pbsys_host_get_event_buf(PBSYS_HOST_TRANSPORT_TYPE_USB, &event_buf, &event_size)) {
                tx_frame_len = pbio_cobs_encode_prefixed(PBIO_PYBRICKS_IN_EP_MSG_EVENT,
                    event_buf, *event_size, tx_frame);

                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_message(&sub, tx_frame, tx_frame_len));
                *event_size = 0;
                if (err != PBIO_SUCCESS) {
                    pbio_usb_reset_state();
                    PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
                }
            } else {
                // Otherwise yield once before going and check again.
                PBIO_OS_AWAIT_ONCE(state);
            }
        }

        PBIO_OS_AWAIT_WHILE(state, pbdrv_usb_is_ready());

        pbio_usb_reset_state();
        PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
    }

    // Unreachable. On cancellation, the charger detection step in the above
    // loop keeps running. It will just skip the data handler.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

void pbio_usb_init(void) {
    pbio_os_process_start(&pbio_usb_process, pbio_usb_process_thread, NULL);
}

void pbio_usb_deinit(void) {
    pbdrv_usb_deinit();
    pbio_os_process_make_request(&pbio_usb_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
}

#endif // PBIO_CONFIG_USB

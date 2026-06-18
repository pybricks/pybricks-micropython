// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "usb.h"

#include <pbdrv/config.h>
#include <pbdrv/usb.h>

#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#include <lwrb/lwrb.h>

#include <string.h>

#include <pbdrv/bluetooth.h>

#include <pbio/version.h>

#include <pbsys/config.h>
#include <pbsys/storage.h>

static pbio_util_void_callback_t pbdrv_usb_host_connection_changed_callback;

void pbdrv_usb_set_host_connection_changed_callback(pbio_util_void_callback_t callback) {
    pbdrv_usb_host_connection_changed_callback = callback;
}

/**
 * Whether the host has opened the serial port (DTR asserted). This is the USB
 * analog of a BLE host subscribing to notifications.
 */
static bool pbdrv_usb_dtr;

bool pbdrv_usb_connection_is_active(void) {
    return pbdrv_usb_is_ready() && pbdrv_usb_dtr;
}

//
// COBS (Consistent Overhead Byte Stuffing) framing.
//
// The host to hub and hub to host directions are raw byte streams (CDC ACM /
// Web Serial), so messages are delimited with a zero byte and COBS-encoded to
// guarantee the payload itself never contains a zero. This is self
// synchronizing: a corrupt or oversized frame is discarded at the next
// delimiter without losing frame alignment.
//
// The delimiter, size macros and the encode/decode helpers live in usb.h so
// the mock simulation drivers can reuse them.
//

/**
 * COBS-encodes @p len bytes from @p src into @p dst and appends the frame
 * delimiter. @p dst must have room for at least ::PBDRV_USB_MAX_ENCODED_MESSAGE_SIZE bytes
 * when @p len is at most ::PBDRV_USB_MAX_DECODED_MESSAGE_SIZE.
 *
 * @return  Number of bytes written to @p dst, including the trailing delimiter.
 */
uint32_t pbdrv_usb_cobs_encode(const uint8_t *src, uint32_t len, uint8_t *dst) {
    uint32_t read_idx = 0;
    uint32_t write_idx = 1;
    uint32_t code_idx = 0;
    uint8_t code = 1;

    while (read_idx < len) {
        if (src[read_idx] == 0) {
            dst[code_idx] = code;
            code = 1;
            code_idx = write_idx++;
            read_idx++;
        } else {
            dst[write_idx++] = src[read_idx++];
            if (++code == 0xFF) {
                dst[code_idx] = code;
                code = 1;
                code_idx = write_idx++;
            }
        }
    }

    dst[code_idx] = code;
    dst[write_idx++] = PBDRV_USB_COBS_DELIMITER;

    return write_idx;
}

/**
 * COBS-decodes @p len bytes from @p src (a single frame with the delimiter
 * already stripped) into @p dst.
 *
 * @return  Number of decoded bytes, or 0 if the frame was empty or malformed.
 */
uint32_t pbdrv_usb_cobs_decode(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t dst_max) {
    uint32_t read_idx = 0;
    uint32_t write_idx = 0;

    while (read_idx < len) {
        uint8_t code = src[read_idx++];

        for (uint8_t i = 1; i < code; i++) {
            if (read_idx >= len || write_idx >= dst_max) {
                // Malformed frame: ran out of input or output space.
                return 0;
            }
            dst[write_idx++] = src[read_idx++];
        }

        if (code < 0xFF && read_idx < len) {
            if (write_idx >= dst_max) {
                return 0;
            }
            dst[write_idx++] = 0;
        }
    }

    return write_idx;
}

/**
 * Pybricks system command handler.
 */
static pbdrv_usb_receive_handler_t pbdrv_usb_receive_handler;

void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler) {
    pbdrv_usb_receive_handler = handler;
}

/** Maximum number of value bytes in a USB read reply message. */
#define PBDRV_USB_READ_MAX_PAYLOAD (PBDRV_USB_MAX_DECODED_MESSAGE_SIZE - 4)

static uint32_t pbdrv_usb_copy_str(uint8_t *buf, const char *str) {
    uint32_t size = strlen(str);
    if (size > PBDRV_USB_READ_MAX_PAYLOAD) {
        size = PBDRV_USB_READ_MAX_PAYLOAD;
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
static uint32_t pbdrv_usb_read_characteristic(uint8_t service, uint16_t char_id, uint8_t *buf) {
    switch (service) {
        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_GATT:
            switch (char_id) {
                case 0x2A00: // Device Name
                    return pbdrv_usb_copy_str(buf, pbdrv_bluetooth_get_hub_name());
                case 0x2A26: // Firmware Revision
                    return pbdrv_usb_copy_str(buf, PBIO_VERSION_STR);
                case 0x2A28: // Software Revision
                    return pbdrv_usb_copy_str(buf, PBIO_PROTOCOL_VERSION_STR);
                default:
                    return 0;
            }
        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_PYBRICKS:
            switch (char_id) {
                case 0x0003: // Hub capabilities
                    pbio_pybricks_hub_capabilities(buf,
                        PBDRV_USB_MAX_DECODED_MESSAGE_SIZE - 1,
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

//
// Functions related to sending value notifications (stdout, status, app data).
//

/**
 * Each event has a buffer sized to the maximum host notification. They are
 * prioritized by ascending event number, so status gets sent first, then
 * stdout, etc.
 */
static uint8_t pbdrv_usb_noti_buf[PBIO_PYBRICKS_EVENT_NUM_EVENTS][PBSYS_CONFIG_HOST_NOTIFICATION_SIZE] __attribute__((aligned(4)));
static uint32_t pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_NUM_EVENTS];

/**
 * Buffer scheduled status.
 */
static uint8_t pbdrv_usb_status_data[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
static bool pbdrv_usb_status_data_pending;

/**
 * Incoming COBS frame assembly buffer (encoded bytes, delimiter excluded).
 */
static uint8_t pbdrv_usb_rx_frame[PBDRV_USB_MAX_ENCODED_MESSAGE_SIZE];
static uint32_t pbdrv_usb_rx_frame_len;
static bool pbdrv_usb_rx_overflow;

void pbdrv_usb_on_dtr_changed(bool dtr) {
    if (dtr == pbdrv_usb_dtr) {
        return;
    }

    pbdrv_usb_dtr = dtr;

    // Drop any partially-assembled incoming frame. Otherwise stray bytes from a
    // previous port opener (e.g. an OS modem probe like ModemManager, which
    // writes AT strings with no frame delimiter) would prepend to and corrupt
    // the first real frame of this new connection.
    pbdrv_usb_rx_frame_len = 0;
    pbdrv_usb_rx_overflow = false;

    if (dtr) {
        // Host just opened the port. Send the current status right away, like
        // the first notification after a BLE host subscribes. Device info is
        // not pushed; the host reads it on demand via read requests.
        pbdrv_usb_status_data_pending = true;
    }

    if (pbdrv_usb_host_connection_changed_callback) {
        pbdrv_usb_host_connection_changed_callback();
    }

    pbio_os_request_poll();
}

void pbdrv_usb_schedule_status_update(const uint8_t *status_msg) {
    // Ignore if message identical to last.
    if (!memcmp(pbdrv_usb_status_data, status_msg, sizeof(pbdrv_usb_status_data))) {
        return;
    }

    // Schedule to send whenever the USB process gets round to it.
    memcpy(pbdrv_usb_status_data, status_msg, sizeof(pbdrv_usb_status_data));
    pbdrv_usb_status_data_pending = true;
    pbio_os_request_poll();
}

/**
 * Buffer for scheduled stdout.
 */
static lwrb_t pbdrv_usb_stdout_ring_buf;

pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size) {

    if (!pbdrv_usb_connection_is_active()) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Buffer data to send it more efficiently even if the caller is only
    // writing one byte at a time.
    if ((*size = lwrb_write(&pbdrv_usb_stdout_ring_buf, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // Poke the process to start tx soon-ish. This way, we can accumulate
    // data bytes before actually transmitting.
    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

uint32_t pbdrv_usb_stdout_tx_available(void) {
    if (!pbdrv_usb_connection_is_active()) {
        return UINT32_MAX;
    }
    return lwrb_get_free(&pbdrv_usb_stdout_ring_buf);
}

bool pbdrv_usb_stdout_tx_is_idle(void) {
    if (!pbdrv_usb_connection_is_active()) {
        return true;
    }
    return lwrb_get_full(&pbdrv_usb_stdout_ring_buf) == 0 && !pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_WRITE_STDOUT];
}

void pbdrv_usb_debug_print(const char *data, size_t len) {

    if (!lwrb_is_ready(&pbdrv_usb_stdout_ring_buf)) {
        return;
    }

    // Buffer result with \r injected before \n.
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            lwrb_write(&pbdrv_usb_stdout_ring_buf, (const uint8_t *)"\r", 1);
        }
        lwrb_write(&pbdrv_usb_stdout_ring_buf, (const uint8_t *)&data[i], 1);
    }

    pbio_os_request_poll();
}

pbio_error_t pbdrv_usb_send_event_notification(pbio_os_state_t *state, pbio_pybricks_event_t event_type, const uint8_t *data, size_t size) {
    PBIO_OS_ASYNC_BEGIN(state);

    if (!pbdrv_usb_connection_is_active()) {
        return PBIO_ERROR_INVALID_OP;
    }

    if (size + 2 > PBIO_ARRAY_SIZE(pbdrv_usb_noti_buf[0]) || event_type >= PBIO_PYBRICKS_EVENT_NUM_EVENTS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Existing notification waiting to be sent first.
    if (pbdrv_usb_noti_size[event_type]) {
        return PBIO_ERROR_BUSY;
    }

    // Copy to local buffer and set size so main thread knows to handle it. The
    // first two bytes (end point message type and event type) are already set.
    pbdrv_usb_noti_size[event_type] = size + 2;
    memcpy(&pbdrv_usb_noti_buf[event_type][2], data, size);
    pbio_os_request_poll();

    // Await until main process has finished sending user data. If it
    // disconnected while sending, this completes as well.
    PBIO_OS_AWAIT_WHILE(state, pbdrv_usb_noti_size[event_type]);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Updates send buffers by draining relevant data buffers and find which event
 * has the highest priority to send.
 */
static bool update_and_get_event_buffer(uint8_t **buf, uint32_t **len) {

    // Prepare status.
    if (pbdrv_usb_status_data_pending) {
        // When a status is pending, drain it here while we write it out,
        // so a new status can be set in the mean time without losing it.
        // This is offset by one for the endpoint message type. The status
        // already starts with the event type.
        memcpy(&pbdrv_usb_noti_buf[PBIO_PYBRICKS_EVENT_STATUS_REPORT][1], pbdrv_usb_status_data, PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
        pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_STATUS_REPORT] = PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE + 1;
        pbdrv_usb_status_data_pending = false;
    }

    // Prepare stdout, drain into chunk of maximum send size.
    if (lwrb_get_full(&pbdrv_usb_stdout_ring_buf) != 0) {
        // Message always starts with endpoint type and event byte, but these are already set.
        if (!pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_WRITE_STDOUT]) {
            pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_WRITE_STDOUT] = 2;
        }
        // Drain ring buffer to send buffer as much as we can.
        uint32_t stdout_free = sizeof(pbdrv_usb_noti_buf[PBIO_PYBRICKS_EVENT_WRITE_STDOUT]) - pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_WRITE_STDOUT];
        if (stdout_free) {
            uint8_t *dest = &pbdrv_usb_noti_buf[PBIO_PYBRICKS_EVENT_WRITE_STDOUT][sizeof(pbdrv_usb_noti_buf[PBIO_PYBRICKS_EVENT_WRITE_STDOUT]) - stdout_free];
            pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_WRITE_STDOUT] += lwrb_read(&pbdrv_usb_stdout_ring_buf, dest, stdout_free);
        }
    }

    // Other events are awaited as-is and don't allow setting new data until
    // they have been transmitted, so don't need further processing/draining.

    // Return highest priority pending event, ready for sending.s
    for (uint32_t i = 0; i < PBIO_PYBRICKS_EVENT_NUM_EVENTS; i++) {
        if (pbdrv_usb_noti_size[i]) {
            *len = &pbdrv_usb_noti_size[i];
            *buf = pbdrv_usb_noti_buf[i];
            return true;
        }
    }
    return false;
}

/**
 * Pending command response to the most recently received command.
 *
 * The host keeps a single command outstanding at a time (like a BLE write with
 * response), so a single command response slot is sufficient.
 */
static uint8_t pbdrv_usb_command_response_buf[sizeof(uint32_t) + 1] = {
    [0] = PBIO_PYBRICKS_IN_EP_MSG_RESPONSE,
};
static bool pbdrv_usb_command_response_pending;

/**
 * Pending reply to the most recently received read request.
 *
 * Like responses, the host keeps a single read outstanding at a time, so a
 * single reply slot is sufficient. The buffer holds the full message
 * `[READ_REPLY, service, char_id_lo, char_id_hi, value...]`.
 */
static uint8_t pbdrv_usb_read_reply_buf[PBDRV_USB_MAX_DECODED_MESSAGE_SIZE];
static uint32_t pbdrv_usb_read_reply_len;
static bool pbdrv_usb_read_reply_pending;

/** Number of header bytes before the value in a read reply. */
#define PBDRV_USB_READ_REPLY_HEADER_SIZE 4

/**
 * Non-blocking poll handler to process incoming bytes.
 *
 * Bytes are assembled into COBS frames. Each completed frame is decoded into a
 * host-to-hub message whose first byte selects the type: a command is handled
 * synchronously and its result queued as a response, while a read request is
 * answered synchronously and queued as a read reply. This never depends on the
 * transmit state, so it cannot deadlock.
 */
static void pbdrv_usb_handle_data_in(void) {

    // Bytes are copied here so the driver can immediately queue the next
    // receive. Only the single USB process thread runs this, so a static
    // scratch buffer is safe and keeps the worst-case packet off the stack.
    static uint8_t data_in[PBDRV_USB_RX_PACKET_MAX_SIZE];
    uint32_t size = pbdrv_usb_get_data_and_start_receive(data_in);

    for (uint32_t i = 0; i < size; i++) {
        uint8_t byte = data_in[i];

        if (byte != PBDRV_USB_COBS_DELIMITER) {
            if (pbdrv_usb_rx_frame_len < sizeof(pbdrv_usb_rx_frame)) {
                pbdrv_usb_rx_frame[pbdrv_usb_rx_frame_len++] = byte;
            } else {
                // Frame too big. Discard until the next delimiter resyncs us.
                pbdrv_usb_rx_overflow = true;
            }
            continue;
        }

        // Delimiter reached: end of frame.
        if (!pbdrv_usb_rx_overflow && pbdrv_usb_rx_frame_len > 0) {
            uint8_t msg[PBDRV_USB_MAX_DECODED_MESSAGE_SIZE];
            uint32_t msg_size = pbdrv_usb_cobs_decode(
                pbdrv_usb_rx_frame, pbdrv_usb_rx_frame_len, msg, sizeof(msg));

            // The first byte is the host-to-hub message type and the rest is
            // its payload.
            if (msg_size >= 2 && msg[0] == PBIO_PYBRICKS_OUT_EP_MSG_COMMAND && pbdrv_usb_receive_handler) {
                // Compute the response synchronously and queue it immediately.
                pbio_set_uint32_le(&pbdrv_usb_command_response_buf[1],
                    pbdrv_usb_receive_handler(&msg[1], msg_size - 1));
                pbdrv_usb_command_response_pending = true;
                pbio_os_request_poll();
            } else if (msg_size >= 4 && msg[0] == PBIO_PYBRICKS_OUT_EP_MSG_READ) {
                // A read request is [type, service, char_id_lo, char_id_hi].
                // Read the value synchronously and queue the reply, echoing the
                // selector so the host can correlate it.
                uint8_t service = msg[1];
                uint16_t char_id = pbio_get_uint16_le(&msg[2]);
                pbdrv_usb_read_reply_buf[0] = PBIO_PYBRICKS_IN_EP_MSG_READ_REPLY;
                pbdrv_usb_read_reply_buf[1] = service;
                pbdrv_usb_read_reply_buf[2] = msg[2];
                pbdrv_usb_read_reply_buf[3] = msg[3];
                uint32_t value_size = pbdrv_usb_read_characteristic(
                    service, char_id, &pbdrv_usb_read_reply_buf[PBDRV_USB_READ_REPLY_HEADER_SIZE]);
                pbdrv_usb_read_reply_len = PBDRV_USB_READ_REPLY_HEADER_SIZE + value_size;
                pbdrv_usb_read_reply_pending = true;
                pbio_os_request_poll();
            }
        }

        pbdrv_usb_rx_frame_len = 0;
        pbdrv_usb_rx_overflow = false;
    }
}

static void pbdrv_usb_reset_state(void) {
    pbdrv_usb_on_dtr_changed(false);
    pbdrv_usb_command_response_pending = false;
    pbdrv_usb_read_reply_pending = false;
    pbdrv_usb_status_data_pending = false;
    pbdrv_usb_rx_frame_len = 0;
    pbdrv_usb_rx_overflow = false;
    lwrb_reset(&pbdrv_usb_stdout_ring_buf);
    memset(pbdrv_usb_noti_size, 0, sizeof(pbdrv_usb_noti_size));
}

static pbio_os_process_t pbdrv_usb_process;

static pbio_error_t pbdrv_usb_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    static uint32_t *noti_size;
    static uint8_t *noti_buf;

    // COBS-encoded frame scratch buffer, populated just before each transmit.
    static uint8_t tx_frame[PBDRV_USB_MAX_ENCODED_MESSAGE_SIZE];
    static uint32_t tx_frame_len;

    pbio_error_t err;

    // Runs every time. If there is no connection, there just won't be data.
    pbdrv_usb_handle_data_in();

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        if (pbdrv_usb_host_connection_changed_callback) {
            pbdrv_usb_host_connection_changed_callback();
        }

        // Run charger detection: wait for USB to become physically plugged in.
        PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_wait_until_configured(&sub));

        while (pbdrv_usb_process.request != PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL && pbdrv_usb_is_ready()) {

            // Find out what we should send, if anything, prioritizing replies
            // to host requests (command response, then read reply), then status, then
            // stdout, then other events. Unlike events, replies do not require
            // an active connection, since they answer a request that was just
            // received.
            if (pbdrv_usb_command_response_pending) {
                tx_frame_len = pbdrv_usb_cobs_encode(pbdrv_usb_command_response_buf, sizeof(pbdrv_usb_command_response_buf), tx_frame);

                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_chunk(&sub, tx_frame, tx_frame_len));
                pbdrv_usb_command_response_pending = false;
                if (err != PBIO_SUCCESS) {
                    pbdrv_usb_reset_state();
                    PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
                }
            } else if (pbdrv_usb_read_reply_pending) {
                // Reply to a characteristic read request.
                tx_frame_len = pbdrv_usb_cobs_encode(pbdrv_usb_read_reply_buf, pbdrv_usb_read_reply_len, tx_frame);

                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_chunk(&sub, tx_frame, tx_frame_len));
                pbdrv_usb_read_reply_pending = false;
                if (err != PBIO_SUCCESS) {
                    pbdrv_usb_reset_state();
                    PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
                }
            } else if (pbdrv_usb_connection_is_active() && update_and_get_event_buffer(&noti_buf, &noti_size)) {
                tx_frame_len = pbdrv_usb_cobs_encode(noti_buf, *noti_size, tx_frame);

                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_chunk(&sub, tx_frame, tx_frame_len));
                *noti_size = 0;
                if (err != PBIO_SUCCESS) {
                    pbdrv_usb_reset_state();
                    PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
                }
            } else {
                // Otherwise yield once before going and check again.
                PBIO_OS_AWAIT_ONCE(state);
            }
        }

        PBIO_OS_AWAIT_WHILE(state, pbdrv_usb_is_ready());

        pbdrv_usb_reset_state();
        PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
    }

    // Unreachable. On cancellation, the charger detection step in the above
    // loop keeps running. It will just skip the data handler.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

void pbdrv_usb_init(void) {
    pbdrv_usb_init_device();

    for (int8_t i = 0; i < PBIO_PYBRICKS_EVENT_NUM_EVENTS; i++) {
        pbdrv_usb_noti_buf[i][0] = PBIO_PYBRICKS_IN_EP_MSG_EVENT;
        pbdrv_usb_noti_buf[i][1] = i; // event type
    }

    // Hardware agnostic buffer before user print waits.
    static uint8_t stdout_buf[512];
    lwrb_init(&pbdrv_usb_stdout_ring_buf, stdout_buf, PBIO_ARRAY_SIZE(stdout_buf));

    pbio_os_process_start(&pbdrv_usb_process, pbdrv_usb_process_thread, NULL);
}

void pbdrv_usb_deinit(void) {
    pbdrv_usb_deinit_device();
    pbio_os_process_make_request(&pbdrv_usb_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
}

#endif // PBDRV_CONFIG_USB

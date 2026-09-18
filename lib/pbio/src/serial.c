// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 The Pybricks Authors

// Runs the Pybricks Profile over COBS-framed serial byte streams such as USB
// CDC and Bluetooth Classic RFCOMM, with one connection process per transport.

#include <pbio/serial.h>

#if PBIO_CONFIG_SERIAL

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/usb.h>

#include <pbio/cobs.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/util.h>

#include <pbsys/command.h>
#include <pbsys/config.h>
#include <pbsys/host.h>
#include <pbsys/status.h>

// Host event size (already includes its 1 event byte) + 1 EP type byte.
#define PBIO_SERIAL_MAX_DECODED_MESSAGE_SIZE (PBSYS_CONFIG_HOST_EVENT_OUT_SIZE + 1)
// The EP type byte is the COBS prefix, so the payload bound excludes it.
#define PBIO_SERIAL_MAX_ENCODED_PACKET_SIZE (PBIO_COBS_ENCODED_BUFFER_SIZE(PBSYS_CONFIG_HOST_EVENT_OUT_SIZE))

/** Number of header bytes before the value in a read reply. */
#define PBIO_SERIAL_READ_REPLY_HEADER_SIZE 3

/**
 * A serial host transport and the connection state of its Pybricks Profile
 * session. The first members are the constant transport driver operations,
 * the rest is runtime state owned by this module.
 */
typedef struct {
    /** The transport backing this connection. */
    pbsys_host_transport_type_t transport;
    /** Reads up to @p size received bytes, returning the number read. */
    uint32_t (*rx_read)(uint8_t *data, uint32_t size);
    /** Awaitable operation to send one encoded frame. */
    pbio_error_t (*tx_message)(pbio_os_state_t *state, const uint8_t *data, uint32_t size);
    /** Awaitable operation to reset the transmit path after an error. Optional. */
    pbio_error_t (*tx_reset)(pbio_os_state_t *state);
    /**
     * Awaitable operation that completes when the transport is ready, with
     * side effects such as USB charger detection. Optional: when not given,
     * simply awaits until is_ready().
     */
    pbio_error_t (*wait_ready)(pbio_os_state_t *state);
    /** Whether the transport is ready, e.g. USB configured or channel open. */
    bool (*is_ready)(void);
    /** De-initializes the transport driver on soft-poweroff. Optional. */
    void (*deinit)(void);

    /** The process running this connection. */
    pbio_os_process_t process;
    /** Child protothread state of awaited driver operations. */
    pbio_os_state_t sub;
    /**
     * Whether the host has opened the port (USB DTR asserted or RFCOMM
     * channel open). This detects disconnection gracefully, even if the host
     * abruptly goes away (e.g. the browser tab is closed) without
     * unsubscribing.
     */
    bool port_open;
    /**
     * Whether the host is subscribed to our outgoing event messages. This is
     * the serial analog of a BLE host subscribing to notifications.
     *
     * Unlike the port state, this is asserted explicitly by the host
     * application rather than automatically by the OS when the port is
     * opened. It gates the initial event flood so that we don't bombard the
     * host with events the moment the port is (possibly automatically)
     * opened.
     */
    bool subscribed;
    /** Incoming COBS frame assembly buffer (encoded bytes, delimiter excluded). */
    uint8_t rx_frame[PBIO_SERIAL_MAX_ENCODED_PACKET_SIZE];
    uint32_t rx_frame_len;
    bool rx_overflow;
    /**
     * Pending command response to the most recently received command.
     *
     * The host keeps a single command outstanding at a time (like a BLE write
     * with response), so a single command response slot is sufficient. The
     * buffer holds the message payload `[tag, status32]`; the RESPONSE
     * message type is added as the COBS prefix at transmit time. `tag` echoes
     * the byte from the command that produced this response so the host can
     * correlate them.
     */
    uint8_t command_response_buf[sizeof(uint8_t) + sizeof(uint32_t) + 1];
    bool command_response_pending;
    /**
     * Pending reply to the most recently received read request.
     *
     * Like responses, the host keeps a single read outstanding at a time, so
     * a single reply slot is sufficient. The buffer holds the message payload
     * `[service, char_id_lo, char_id_hi, value...]`; the READ_REPLY message
     * type is added as the COBS prefix at transmit time.
     */
    uint8_t read_reply_buf[PBIO_SERIAL_MAX_DECODED_MESSAGE_SIZE];
    uint32_t read_reply_len;
    bool read_reply_pending;
    /** Staged event shared with pbsys host, and this transport's size latch. */
    uint8_t *event_buf;
    uint32_t *event_size;
    /** COBS-encoded frame scratch buffer, populated just before each transmit. */
    uint8_t tx_frame[PBIO_SERIAL_MAX_ENCODED_PACKET_SIZE];
    uint32_t tx_frame_len;
} pbio_serial_connection_t;

static pbio_serial_connection_t pbio_serial_connections[] = {
    #if PBIO_CONFIG_USB
    {
        .transport = PBSYS_HOST_TRANSPORT_TYPE_USB,
        .rx_read = pbdrv_usb_rx_read,
        .tx_message = pbdrv_usb_tx_message,
        .tx_reset = pbdrv_usb_tx_reset,
        // Runs charger detection while awaiting physical plug-in.
        .wait_ready = pbdrv_usb_wait_until_configured,
        .is_ready = pbdrv_usb_is_ready,
        // Keeps charging if supported, so has an explicit deinit.
        .deinit = pbdrv_usb_deinit,
    },
    #endif // PBIO_CONFIG_USB
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC
    {
        .transport = PBSYS_HOST_TRANSPORT_TYPE_RFCOMM,
        .rx_read = pbdrv_bluetooth_classic_host_rx_read,
        .tx_message = pbdrv_bluetooth_classic_host_tx_message,
        // Readiness coincides with the channel being open. The Bluetooth
        // process owns the chip lifecycle, so there is nothing else to await
        // or reset here, but the link is dropped on soft-poweroff so that the
        // host sees a disconnect rather than a silent timeout.
        .is_ready = pbdrv_bluetooth_classic_host_is_connected,
        .deinit = pbdrv_bluetooth_classic_host_disconnect,
    },
    #endif // PBDRV_CONFIG_BLUETOOTH_CLASSIC
};

static pbio_serial_connection_t *pbio_serial_get_connection(pbsys_host_transport_type_t transport) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(pbio_serial_connections); i++) {
        if (pbio_serial_connections[i].transport == transport) {
            return &pbio_serial_connections[i];
        }
    }
    return NULL;
}

static bool pbio_serial_connection_is_active_con(pbio_serial_connection_t *con) {
    return con->is_ready() && con->port_open && con->subscribed;
}

bool pbio_serial_connection_is_active(pbsys_host_transport_type_t transport) {
    pbio_serial_connection_t *con = pbio_serial_get_connection(transport);
    return con != NULL && pbio_serial_connection_is_active_con(con);
}

void pbio_serial_port_changed(pbsys_host_transport_type_t transport, bool open) {
    pbio_serial_connection_t *con = pbio_serial_get_connection(transport);
    if (con == NULL || open == con->port_open) {
        return;
    }

    con->port_open = open;

    // Drop any partially-assembled incoming frame. Otherwise stray bytes from a
    // previous port opener (e.g. an OS modem probe like ModemManager, which
    // writes AT strings with no frame delimiter) would prepend to and corrupt
    // the first real frame of this new connection.
    con->rx_frame_len = 0;
    con->rx_overflow = false;

    if (!open) {
        // Host closed the port. The subscription implicitly falls with it. In
        // practice the host rarely unsubscribes explicitly; it just closes the
        // port, which we detect even if the host abruptly goes away.
        con->subscribed = false;
    }

    pbsys_host_connection_changed();

    pbio_os_request_poll();
}

/**
 * Sets whether the host is subscribed to event notifications and notifies
 * listeners of the connection state change.
 */
static void pbio_serial_set_subscribed(pbio_serial_connection_t *con, bool subscribed) {
    if (subscribed == con->subscribed) {
        return;
    }

    con->subscribed = subscribed;

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
 * Non-blocking poll handler to process incoming bytes.
 *
 * Bytes are assembled into COBS frames. Each completed frame is decoded into a
 * host-to-hub message whose first byte selects the type: a command is handled
 * synchronously and its result queued as a response, while a read request is
 * answered synchronously and queued as a read reply. This never depends on the
 * transmit state, so it cannot deadlock.
 */
static void pbio_serial_handle_data_in(pbio_serial_connection_t *con) {

    // Bytes are copied here so the driver can immediately free up buffer
    // space to receive more. All connection processes run to completion on a
    // single thread and this scratch data does not persist across calls, so
    // sharing static buffers is safe and keeps the worst-case read off the
    // stack while only occupying the memory once.
    static uint8_t data_in[PBIO_SERIAL_MAX_ENCODED_PACKET_SIZE];
    static uint8_t msg[PBIO_SERIAL_MAX_DECODED_MESSAGE_SIZE];
    uint32_t size;

    while ((size = con->rx_read(data_in, sizeof(data_in))) > 0) {
        for (uint32_t i = 0; i < size; i++) {
            uint8_t byte = data_in[i];

            if (byte != PBIO_COBS_DELIMITER) {
                if (con->rx_frame_len < sizeof(con->rx_frame)) {
                    con->rx_frame[con->rx_frame_len++] = byte;
                } else {
                    // Frame too big. Discard until the next delimiter resyncs us.
                    con->rx_overflow = true;
                }
                continue;
            }

            // Delimiter reached: end of frame.
            if (!con->rx_overflow && con->rx_frame_len > 0) {
                uint8_t msg_type;
                uint32_t msg_size = pbio_cobs_decode_prefixed(
                    con->rx_frame, con->rx_frame_len, &msg_type, msg, sizeof(msg));

                // The decoded prefix is the host-to-hub message type and the rest
                // is its payload.
                if (msg_size >= 1 && msg_type == PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE) {
                    // Subscribe or unsubscribe to event notifications. The payload
                    // is a single byte: 1 to subscribe, 0 to unsubscribe.
                    pbio_serial_set_subscribed(con, msg[0]);
                } else if (msg_size >= 2 && msg_type == PBIO_PYBRICKS_OUT_EP_MSG_COMMAND) {
                    // The command payload is [tag, ...payload]. The tag is opaque
                    // to us: echo it back in the response so the host can correlate
                    // a late response with the command that produced it. The
                    // payload after the tag is the same as a BLE command write.
                    con->command_response_buf[0] = msg[0];
                    pbio_set_uint32_le(&con->command_response_buf[1],
                        pbsys_handle_command(&msg[1], msg_size - 1));
                    con->command_response_pending = true;
                    pbio_os_request_poll();
                } else if (msg_size >= 3 && msg_type == PBIO_PYBRICKS_OUT_EP_MSG_READ) {
                    // A read request payload is [service, char_id_lo, char_id_hi].
                    // Read the value synchronously and queue the reply, echoing the
                    // selector so the host can correlate it.
                    uint8_t service = msg[0];
                    uint16_t char_id = pbio_get_uint16_le(&msg[1]);
                    con->read_reply_buf[0] = service;
                    con->read_reply_buf[1] = msg[1];
                    con->read_reply_buf[2] = msg[2];
                    // The value bound excludes the EP type byte and the reply header.
                    uint32_t value_size = pbsys_host_read_characteristic(
                        service, char_id, con->transport,
                        &con->read_reply_buf[PBIO_SERIAL_READ_REPLY_HEADER_SIZE],
                        PBIO_SERIAL_MAX_DECODED_MESSAGE_SIZE - 1 - PBIO_SERIAL_READ_REPLY_HEADER_SIZE);
                    con->read_reply_len = PBIO_SERIAL_READ_REPLY_HEADER_SIZE + value_size;
                    con->read_reply_pending = true;
                    pbio_os_request_poll();
                }
            }

            con->rx_frame_len = 0;
            con->rx_overflow = false;
        }
    }
}

static void pbio_serial_reset_state(pbio_serial_connection_t *con) {
    pbio_serial_port_changed(con->transport, false);
    con->subscribed = false;
    con->command_response_pending = false;
    con->read_reply_pending = false;
    con->rx_frame_len = 0;
    con->rx_overflow = false;
}

static pbio_error_t pbio_serial_process_thread(pbio_os_state_t *state, void *context) {

    pbio_serial_connection_t *con = context;

    pbio_error_t err;

    // Runs every time. If there is no connection, there just won't be data.
    pbio_serial_handle_data_in(con);

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        pbsys_host_connection_changed();

        // Wait for the transport to become ready, e.g. USB physically plugged
        // in and configured, or the RFCOMM channel opened by the host.
        if (con->wait_ready) {
            PBIO_OS_AWAIT(state, &con->sub, err = con->wait_ready(&con->sub));
        } else {
            PBIO_OS_AWAIT_UNTIL(state, con->is_ready());
        }

        while (con->process.request != PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL && con->is_ready()) {

            // Find out what we should send, if anything, prioritizing replies
            // to host requests (command response, then read reply), then status, then
            // stdout, then other events. Unlike events, replies do not require
            // an active connection, since they answer a request that was just
            // received.
            if (con->command_response_pending) {
                con->tx_frame_len = pbio_cobs_encode_prefixed(PBIO_PYBRICKS_IN_EP_MSG_RESPONSE,
                    con->command_response_buf, sizeof(con->command_response_buf) - 1, con->tx_frame);

                PBIO_OS_AWAIT(state, &con->sub, err = con->tx_message(&con->sub, con->tx_frame, con->tx_frame_len));
                con->command_response_pending = false;
                if (err != PBIO_SUCCESS) {
                    pbio_serial_reset_state(con);
                    if (con->tx_reset) {
                        PBIO_OS_AWAIT(state, &con->sub, con->tx_reset(&con->sub));
                    }
                }
            } else if (con->read_reply_pending) {
                // Reply to a characteristic read request.
                con->tx_frame_len = pbio_cobs_encode_prefixed(PBIO_PYBRICKS_IN_EP_MSG_READ_REPLY,
                    con->read_reply_buf, con->read_reply_len, con->tx_frame);

                PBIO_OS_AWAIT(state, &con->sub, err = con->tx_message(&con->sub, con->tx_frame, con->tx_frame_len));
                con->read_reply_pending = false;
                if (err != PBIO_SUCCESS) {
                    pbio_serial_reset_state(con);
                    if (con->tx_reset) {
                        PBIO_OS_AWAIT(state, &con->sub, con->tx_reset(&con->sub));
                    }
                }
            } else if (pbio_serial_connection_is_active_con(con) && pbsys_host_get_event_buf(con->transport, &con->event_buf, &con->event_size)) {
                con->tx_frame_len = pbio_cobs_encode_prefixed(PBIO_PYBRICKS_IN_EP_MSG_EVENT,
                    con->event_buf, *con->event_size, con->tx_frame);

                PBIO_OS_AWAIT(state, &con->sub, err = con->tx_message(&con->sub, con->tx_frame, con->tx_frame_len));
                *con->event_size = 0;
                if (err != PBIO_SUCCESS) {
                    pbio_serial_reset_state(con);
                    if (con->tx_reset) {
                        PBIO_OS_AWAIT(state, &con->sub, con->tx_reset(&con->sub));
                    }
                }
            } else {
                // Otherwise yield once before going and check again.
                PBIO_OS_AWAIT_ONCE(state);
            }
        }

        PBIO_OS_AWAIT_WHILE(state, con->is_ready());

        pbio_serial_reset_state(con);
        if (con->tx_reset) {
            PBIO_OS_AWAIT(state, &con->sub, con->tx_reset(&con->sub));
        }
    }

    // Unreachable. On cancellation, the wait-ready step in the above loop
    // keeps running, e.g. for USB charger detection. It will just skip the
    // data handler.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

void pbio_serial_init(void) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(pbio_serial_connections); i++) {
        pbio_serial_connection_t *con = &pbio_serial_connections[i];
        pbio_os_process_start(&con->process, pbio_serial_process_thread, con);
    }
}

void pbio_serial_deinit(void) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(pbio_serial_connections); i++) {
        pbio_serial_connection_t *con = &pbio_serial_connections[i];
        if (con->deinit) {
            con->deinit();
        }
        pbio_os_process_make_request(&con->process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
    }
}

#endif // PBIO_CONFIG_SERIAL

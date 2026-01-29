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

static pbio_util_void_callback_t pbdrv_usb_host_connection_changed_callback;

void pbdrv_usb_set_host_connection_changed_callback(pbio_util_void_callback_t callback) {
    pbdrv_usb_host_connection_changed_callback = callback;
}
/**
 * Host is subscribed to our outgoing event messages.
 */
static bool pbdrv_usb_events_subscribed;

static void pbdrv_usb_events_subscribed_set(bool subscribed) {
    pbdrv_usb_events_subscribed = subscribed;
    if (pbdrv_usb_host_connection_changed_callback) {
        pbdrv_usb_host_connection_changed_callback();
    }
}

bool pbdrv_usb_connection_is_active(void) {
    return pbdrv_usb_events_subscribed && pbdrv_usb_is_ready();
}

/**
 * Pybricks system command handler.
 */
static pbdrv_usb_receive_handler_t pbdrv_usb_receive_handler;

void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler) {
    pbdrv_usb_receive_handler = handler;
}

//
// Functions related to sending value notifications (stdout, status, app data).
//

/**
 * Each event has a buffer of maximum packet size. They are prioritized by
 * ascending event number, so status gets sent first, then stdout, etc.
 */
static uint8_t pbdrv_usb_noti_buf[PBIO_PYBRICKS_EVENT_NUM_EVENTS][PBDRV_CONFIG_USB_MAX_PACKET_SIZE] __attribute__((aligned(4)));
static uint32_t pbdrv_usb_noti_size[PBIO_PYBRICKS_EVENT_NUM_EVENTS];

/**
 * Buffer scheduled status.
 */
static uint8_t pbdrv_usb_status_data[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
static bool pbdrv_usb_status_data_pending;

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



static bool pbdrv_usb_respond_soon;
static pbio_pybricks_error_t pbdrv_usb_respond_result;

/**
 * Non-blocking poll handler to process pending incoming messages.
 */
static void pbdrv_usb_handle_data_in(void) {

    // Ignore incoming data if we haven't sent our previous response yet.
    if (pbdrv_usb_respond_soon) {
        return;
    }

    // Data is copied here so the driver can immediately clear it and queue
    // the next receive.
    uint8_t data_in[PBDRV_CONFIG_USB_MAX_PACKET_SIZE];
    uint32_t size = pbdrv_usb_get_data_and_start_receive(data_in);

    // Expecting at least EP_MSG and payload.
    if (size < 2) {
        return;
    }

    switch (data_in[0]) {
        case PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE:
            pbdrv_usb_events_subscribed_set(data_in[1]);
            pbdrv_usb_respond_result = PBIO_PYBRICKS_ERROR_OK;
            pbdrv_usb_respond_soon = true;

            // Schedule sending current status immediately after subscribing.
            pbdrv_usb_status_data_pending = true;
            break;
        case PBIO_PYBRICKS_OUT_EP_MSG_COMMAND:
            if (pbdrv_usb_receive_handler) {
                pbdrv_usb_respond_result = pbdrv_usb_receive_handler(data_in + 1, size - 1);
                pbdrv_usb_respond_soon = true;
            }
            break;
    }
}

static void pbdrv_usb_reset_state(void) {
    pbdrv_usb_events_subscribed_set(false);
    pbdrv_usb_respond_soon = false;
    pbdrv_usb_status_data_pending = false;
    lwrb_reset(&pbdrv_usb_stdout_ring_buf);
    memset(pbdrv_usb_noti_size, 0, sizeof(pbdrv_usb_noti_size));
}

static pbio_os_process_t pbdrv_usb_process;

static pbio_error_t pbdrv_usb_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    static uint32_t *noti_size;
    static uint8_t *noti_buf;

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

            // Find out what we should send, if anything, priotizing response, then
            // status, then stdout, then other events.
            if (pbdrv_usb_respond_soon) {
                // Double buffer response so we're ready for another.
                static pbio_pybricks_error_t error_code;
                error_code = pbdrv_usb_respond_result;
                pbdrv_usb_respond_soon = false;

                // Send the response.
                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_response(&sub, error_code));
                if (err != PBIO_SUCCESS) {
                    pbdrv_usb_reset_state();
                    PBIO_OS_AWAIT(state, &sub, pbdrv_usb_tx_reset(&sub));
                }
            } else if (pbdrv_usb_connection_is_active() && update_and_get_event_buffer(&noti_buf, &noti_size)) {
                // Send out pending event if any.
                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx_event(&sub, noti_buf, *noti_size));
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

    static uint8_t stdout_buf[PBDRV_CONFIG_USB_MAX_PACKET_SIZE * PBDRV_CONFIG_USB_NUM_BUFFERED_PACKETS];
    lwrb_init(&pbdrv_usb_stdout_ring_buf, stdout_buf, PBIO_ARRAY_SIZE(stdout_buf));

    pbio_os_process_start(&pbdrv_usb_process, pbdrv_usb_process_thread, NULL);
}

void pbdrv_usb_deinit(void) {
    pbdrv_usb_deinit_device();
    pbio_os_process_make_request(&pbdrv_usb_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
}

#endif // PBDRV_CONFIG_USB

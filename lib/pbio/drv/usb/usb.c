// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

#include <errno.h>
#include <signal.h>
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

/**
 * Host is subscribed to our outgoing event messages.
 */
static bool pbdrv_usb_events_subscribed;

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

    // Schedule to send whenever the Bluetooth process gets round to it.
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
    return lwrb_get_full(&pbdrv_usb_stdout_ring_buf) == 0;
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
            pbdrv_usb_events_subscribed = data_in[1];
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
    pbdrv_usb_events_subscribed = false;
    pbdrv_usb_respond_soon = false;
    pbdrv_usb_status_data_pending = false;
    lwrb_reset(&pbdrv_usb_stdout_ring_buf);
}

static pbio_os_process_t pbdrv_usb_process;

static pbio_error_t pbdrv_usb_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    static uint8_t *out_data;
    static uint32_t out_size;

    pbio_error_t err;

    // Runs every time. If there is no connection, there just won't be data.
    pbdrv_usb_handle_data_in();

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        // Run charger detection: wait for USB to become physically plugged in.
        PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_wait_for_charger(&sub));

        while (pbdrv_usb_process.request != PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL && pbdrv_usb_is_ready()) {

            // Find out what we should send, if anything, priotizing response, then
            // status, then stdout, then other events.
            if (pbdrv_usb_respond_soon) {
                // Pack the response to the most recent message.
                pbdrv_usb_tx_get_buf(PBIO_PYBRICKS_IN_EP_MSG_RESPONSE, &out_data);
                pbio_set_uint32_le(&out_data[1], pbdrv_usb_respond_result);
                out_size = sizeof(uint32_t) + 1;
                pbdrv_usb_respond_soon = false;
            } else if (pbdrv_usb_connection_is_active() && pbdrv_usb_status_data_pending) {
                // Send out status if pending (already includes event code).
                pbdrv_usb_tx_get_buf(PBIO_PYBRICKS_IN_EP_MSG_EVENT, &out_data);
                memcpy(&out_data[1], pbdrv_usb_status_data, PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
                out_size = PBIO_PYBRICKS_USB_MESSAGE_SIZE(PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
                pbdrv_usb_status_data_pending = false;
            } else if (pbdrv_usb_connection_is_active() && lwrb_get_full(&pbdrv_usb_stdout_ring_buf) != 0) {
                // Send out stdout if anything is buffered.
                uint32_t max_size = pbdrv_usb_tx_get_buf(PBIO_PYBRICKS_IN_EP_MSG_EVENT, &out_data);
                out_data[1] = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;
                out_size = lwrb_read(&pbdrv_usb_stdout_ring_buf, &out_data[2], max_size - 2) + 2;
            }

            // If there was anything to send, send it.
            if (out_size) {
                PBIO_OS_AWAIT(state, &sub, err = pbdrv_usb_tx(&sub, out_data, out_size));
                out_size = 0;
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
        pbdrv_usb_tx_reset(&sub);
    }

    // Unreachable. On cancellation, the charger detection step in the above
    // loop keeps running. It will just skip the data handler.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

void pbdrv_usb_init(void) {
    pbdrv_usb_init_device();

    static uint8_t stdout_buf[PBDRV_CONFIG_USB_MAX_PACKET_SIZE * 2];
    lwrb_init(&pbdrv_usb_stdout_ring_buf, stdout_buf, PBIO_ARRAY_SIZE(stdout_buf));

    pbio_os_process_start(&pbdrv_usb_process, pbdrv_usb_process_thread, NULL);
}

void pbdrv_usb_deinit(void) {
    pbdrv_usb_deinit_device();
    pbio_os_process_make_request(&pbdrv_usb_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
}

#endif // PBDRV_CONFIG_USB

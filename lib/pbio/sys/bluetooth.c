// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_BLUETOOTH

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <contiki-lib.h>
#include <contiki.h>
#include <lwrb/lwrb.h>

#include <pbdrv/bluetooth.h>
#include <pbio/error.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbsys/host.h>
#include <pbsys/command.h>
#include <pbsys/status.h>
#include <pbsys/storage.h>

#include "hmi.h"
#include "storage.h"

// REVISIT: this can be the negotiated MTU - 3 to allow for better throughput
#define MAX_CHAR_SIZE 20

static lwrb_t stdout_ring_buf;

typedef struct {
    list_t queue;
    pbdrv_bluetooth_send_context_t context;
    bool is_queued;
    uint8_t payload[MAX_CHAR_SIZE];
} send_msg_t;

static send_msg_t stdout_msg;
LIST(send_queue);
static bool send_busy;

PROCESS(pbsys_bluetooth_process, "Bluetooth");

void pbsys_bluetooth_process_poll(void) {
    process_poll(&pbsys_bluetooth_process);
}

// Internal API

/** Initializes Bluetooth. */
void pbsys_bluetooth_init(void) {
    // enough for two packets, one currently being sent and one to be ready
    // as soon as the previous one completes + 1 byte for ring buf pointer
    static uint8_t stdout_buf[MAX_CHAR_SIZE * 2 + 1];

    lwrb_init(&stdout_ring_buf, stdout_buf, PBIO_ARRAY_SIZE(stdout_buf));

    process_start(&pbsys_bluetooth_process);
}

// Public API

/**
 * Queues data to be transmitted via Bluetooth serial port.
 * @param data  [in]        The data to be sent.
 * @param size  [in, out]   The size of @p data in bytes. After return, @p size
 *                          contains the number of bytes actually written.
 * @return                  ::PBIO_SUCCESS if some @p data was queued,
 *                          ::PBIO_ERROR_AGAIN if no @p data could be
 *                          queued at this time (e.g. buffer is full),
 *                          ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support Bluetooth.
 */
pbio_error_t pbsys_bluetooth_tx(const uint8_t *data, uint32_t *size) {

    // make sure we have a Bluetooth connection
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // only allow one UART Tx message in the queue at a time
    if (!stdout_msg.is_queued) {
        // Setting data and size are deferred until we actually send the message.
        // This way, if the caller is only writing one byte at a time, we can
        // still buffer data to send it more efficiently.
        stdout_msg.context.connection = PBDRV_BLUETOOTH_CONNECTION_PYBRICKS;

        list_add(send_queue, &stdout_msg);
        stdout_msg.is_queued = true;
    }

    if ((*size = lwrb_write(&stdout_ring_buf, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // poke the process to start tx soon-ish. This way, we can accumulate up to
    // MAX_CHAR_SIZE bytes before actually transmitting
    pbsys_bluetooth_process_poll();

    return PBIO_SUCCESS;
}

/**
 * Gets the number of bytes that can be queued for transmission via Bluetooth.
 *
 * If there is no connection, this will return %UINT32_MAX.
 *
 * @returns The number of bytes that can be queued.
 */
uint32_t pbsys_bluetooth_tx_available(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
        return UINT32_MAX;
    }

    return lwrb_get_free(&stdout_ring_buf);
}

/**
 * Tests if the Tx queue is empty and all data has been sent over the air.
 *
 * If there is no connection, this will always return @c true.
 *
 * @returns @c true if the condition is met, otherwise @c false.
 */
bool pbsys_bluetooth_tx_is_idle(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
        return true;
    }

    return !send_busy && lwrb_get_full(&stdout_ring_buf) == 0;
}

// Contiki process

static pbio_pybricks_error_t handle_receive(pbdrv_bluetooth_connection_t connection, const uint8_t *data, uint32_t size) {
    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS) {
        return pbsys_command(data, size);
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_UART) {
        // TODO: need a new buffer for NUS
        // lwrb_write(&uart_rx_ring, data, size);
        return PBIO_PYBRICKS_ERROR_OK;
    }

    return PBIO_PYBRICKS_ERROR_INVALID_HANDLE;
}

static void send_done(void) {
    send_msg_t *msg = list_pop(send_queue);

    if (msg == &stdout_msg && lwrb_get_full(&stdout_ring_buf)) {
        // If there is more buffered data to send, put the message back in the queue
        list_add(send_queue, msg);
    } else {
        msg->is_queued = false;
    }

    send_busy = false;
    pbsys_bluetooth_process_poll();
}

// drain all buffers and queues and reset global state
static void reset_all(void) {
    send_msg_t *msg;

    while ((msg = list_pop(send_queue))) {
        msg->is_queued = false;
    }

    send_busy = false;

    lwrb_reset(&stdout_ring_buf);
}

static PT_THREAD(pbsys_bluetooth_monitor_status(struct pt *pt)) {
    static struct etimer timer;
    static uint32_t old_status_flags;
    static send_msg_t msg;
    #if PBSYS_CONFIG_HMI_NUM_SLOTS
    static uint8_t old_program_slot;
    #endif

    PT_BEGIN(pt);

    // This ensures that we always send a notification with the current status
    // right after notifications are enabled.
    old_status_flags = ~0;

    // Send status periodically as well in case a notification was missed due
    // to bad RF environment or bug like https://crbug.com/1195592
    etimer_set(&timer, 500);

    for (;;) {
        // wait for status to change or timeout
        PT_WAIT_UNTIL(pt, pbsys_status_get_flags() != old_status_flags ||
            #if PBSYS_CONFIG_HMI_NUM_SLOTS
            pbsys_hmi_get_selected_program_slot() != old_program_slot ||
            #endif
            etimer_expired(&timer));

        etimer_restart(&timer);

        // send the message
        _Static_assert(sizeof(msg.payload) >= PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
        msg.context.size = pbsys_status_get_status_report(&msg.payload[0]);
        msg.context.connection = PBDRV_BLUETOOTH_CONNECTION_PYBRICKS;
        list_add(send_queue, &msg);
        msg.is_queued = true;
        old_status_flags = pbsys_status_get_flags();

        // wait for message to be sent - note: it is possible to miss status changes
        // if the status changes and then changes back to old_status_flags while we
        // are waiting.
        PT_WAIT_WHILE(pt, msg.is_queued);
    }

    PT_END(pt);
}

PROCESS_THREAD(pbsys_bluetooth_process, ev, data) {
    static struct pt status_monitor_pt;

    PROCESS_BEGIN();

    pbdrv_bluetooth_set_on_event(pbsys_bluetooth_process_poll);
    pbdrv_bluetooth_set_receive_handler(handle_receive);

    while (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {

        // Now we are idle. We need to change the Bluetooth state and
        // indicators if a host connects to us, or a user program starts, or we
        // shut down, or Bluetooth is disabled by the user.
        PROCESS_WAIT_UNTIL(
            pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)
            );

        PT_INIT(&status_monitor_pt);

        // The Bluetooth enabled flag can only change while disconnected and
        // while no program is running. So here it just serves to skip the
        // Bluetooth loop below and go directly to the disable step below it.
        while (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)
               && !pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {

            if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
                // Since pbsys status events are broadcast to all processes, this
                // will get triggered right away if there is a status change event.
                pbsys_bluetooth_monitor_status(&status_monitor_pt);
            } else {
                // REVISIT: this is probably a bit inefficient since it only
                // needs to be called once each time notifications are enabled
                PT_INIT(&status_monitor_pt);
            }

            if (!send_busy) {
                // msg is removed from queue in send_done callback rather than here
                send_msg_t *msg = list_head(send_queue);
                if (msg) {
                    msg->context.done = send_done;

                    if (msg == &stdout_msg) {
                        msg->payload[0] = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;
                        // REVISIT: use negotiated MTU instead of minimum safe size
                        msg->context.size = lwrb_read(&stdout_ring_buf, &msg->payload[1], PBIO_ARRAY_SIZE(msg->payload) - 1) + 1;
                        assert(msg->context.size > 1);
                    }

                    msg->context.data = &msg->payload[0];
                    send_busy = true;
                    pbdrv_bluetooth_send(&msg->context);
                }
            }

            PROCESS_WAIT_EVENT();
        }

        reset_all();
    }

    PROCESS_END();
}

#endif // PBSYS_CONFIG_BLUETOOTH

// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

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
#include <pbio/event.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>

// REVISIT: this can be the negotiated MTU - 3 to allow for better throughput
// max data size for nRF UART characteristics
#define NRF_CHAR_SIZE 20

// nRF UART Rx hook
static pbsys_user_program_stdin_event_callback_t uart_rx_callback;
// ring buffers for UART service
static lwrb_t uart_tx_ring;
static lwrb_t uart_rx_ring;

typedef struct {
    list_t queue;
    pbdrv_bluetooth_tx_context_t context;
    bool is_queued;
    uint8_t payload[NRF_CHAR_SIZE];
} tx_msg_t;

LIST(tx_queue);
static bool tx_busy;

PROCESS(pbsys_bluetooth_process, "Bluetooth");

/** Initializes Bluetooth. */
void pbsys_bluetooth_init(void) {
    static uint8_t uart_tx_buf[NRF_CHAR_SIZE * 2 + 1];
    static uint8_t uart_rx_buf[100 + 1]; // download chunk size

    lwrb_init(&uart_tx_ring, uart_tx_buf, PBIO_ARRAY_SIZE(uart_tx_buf));
    lwrb_init(&uart_rx_ring, uart_rx_buf, PBIO_ARRAY_SIZE(uart_rx_buf));
    process_start(&pbsys_bluetooth_process, NULL);
}

static void on_event(void) {
    process_poll(&pbsys_bluetooth_process);
}

/**
 * Sets the UART Rx callback function.
 * @param callback  [in]    The callback or NULL.
 */
void pbsys_bluetooth_rx_set_callback(pbsys_user_program_stdin_event_callback_t callback) {
    uart_rx_callback = callback;
}

/**
 * Gets the number of bytes currently available to be read from the UART Rx
 * characteristic.
 * @return              The number of bytes.
 */
uint32_t pbsys_bluetooth_rx_get_available(void) {
    return lwrb_get_full(&uart_rx_ring);
}

/**
 * Reads data from the UART Rx characteristic.
 * @param data  [in]        A buffer to receive a copy of the data.
 * @param size  [in, out]   The number of bytes to read (@p data must be at least
 *                          this big). After return @p size contains the number
 *                          of bytes actually read.
 * @return                  ::PBIO_SUCCESS if @p data was read, ::PBIO_ERROR_AGAIN
 *                          if @p data could not be read at this time (i.e. buffer
 *                          is empty), ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support Bluetooth.
 */
pbio_error_t pbsys_bluetooth_rx(uint8_t *data, uint32_t *size) {
    // make sure we have a Bluetooth connection
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_UART)) {
        return PBIO_ERROR_INVALID_OP;
    }

    if ((*size = lwrb_read(&uart_rx_ring, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Queues data to be transmitted via Bluetooth serial port.
 * @param data  [in]        The data to be sent.
 * @param size  [in, out]   The size of @p data in bytes. After return, @p size
 *                          contains the number of bytes actually written.
 * @return                  ::PBIO_SUCCESS if @p data was queued, ::PBIO_ERROR_AGAIN
 *                          if @p data could not be queued at this time (e.g. buffer
 *                          is full), ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support Bluetooth.
 */
pbio_error_t pbsys_bluetooth_tx(const uint8_t *data, uint32_t *size) {
    static tx_msg_t uart_msg;

    // make sure we have a Bluetooth connection
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_UART)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // only allow one UART Tx message in the queue at a time
    if (!uart_msg.is_queued) {
        // Setting data and size are deferred until we actually send the message.
        // This way, if the caller is only writting one byte at a time, we can
        // still buffer data to send it more efficiently.
        uart_msg.context.connection = PBDRV_BLUETOOTH_CONNECTION_UART;

        list_add(tx_queue, &uart_msg);
        uart_msg.is_queued = true;
    }

    if ((*size = lwrb_write(&uart_tx_ring, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // poke the process to start tx soon-ish. This way, we can accumulate up to
    // NRF_CHAR_SIZE bytes before actually transmitting
    process_poll(&pbsys_bluetooth_process);

    return PBIO_SUCCESS;
}

static void handle_rx(pbdrv_bluetooth_connection_t connection, const uint8_t *data, uint8_t size) {
    static tx_msg_t pybricks_msg;

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS) {
        // TODO: this is a temporary echo service - needs to trigger events instead

        if (pybricks_msg.is_queued) {
            return;
        }

        pybricks_msg.context.size = MIN(size, PBIO_ARRAY_SIZE(pybricks_msg.payload));
        for (int i = 0; i < pybricks_msg.context.size; i++) {
            pybricks_msg.payload[i] = data[i] + 1;
        }
        pybricks_msg.context.connection = PBDRV_BLUETOOTH_CONNECTION_PYBRICKS;

        list_add(tx_queue, &pybricks_msg);
        pybricks_msg.is_queued = true;

        process_poll(&pbsys_bluetooth_process);
    } else if (connection == PBDRV_BLUETOOTH_CONNECTION_UART) {
        // This will drop data if buffer is full
        if (uart_rx_callback) {
            // If there is a callback hook, we have to process things one byte at
            // a time.
            for (int i = 0; i < size; i++) {
                if (!uart_rx_callback(data[i])) {
                    lwrb_write(&uart_rx_ring, &data[i], 1);
                }
            }
        } else {
            lwrb_write(&uart_rx_ring, data, size);
        }
    }
}

static void tx_done(pbdrv_bluetooth_tx_context_t *context) {
    tx_msg_t *msg = PBIO_CONTAINER_OF(context, tx_msg_t, context);

    if (msg->context.connection == PBDRV_BLUETOOTH_CONNECTION_UART && lwrb_get_full(&uart_tx_ring)) {
        // If there is more buffered data to send, put the message back in the queue
        list_add(tx_queue, msg);
    } else {
        msg->is_queued = false;
    }

    tx_busy = false;
    process_poll(&pbsys_bluetooth_process);
}

// drain all buffers and queues and reset global state
static void reset_all(void) {
    tx_msg_t *msg;

    while ((msg = list_pop(tx_queue))) {
        msg->is_queued = false;
    }

    tx_busy = false;

    lwrb_reset(&uart_rx_ring);
    lwrb_reset(&uart_tx_ring);
}

static PT_THREAD(pbsys_bluetooth_monitor_status(struct pt *pt)) {
    static uint32_t old_status_flags, new_status_flags;
    static tx_msg_t msg;

    PT_BEGIN(pt);

    old_status_flags = 0;

    for (;;) {
        // wait for status to change
        PT_WAIT_UNTIL(pt, (new_status_flags = pbsys_status_get_flags()) != old_status_flags);

        // send the message
        msg.context.size = pbio_pybricks_event_status_report(&msg.payload[0], new_status_flags);
        msg.context.connection = PBDRV_BLUETOOTH_CONNECTION_PYBRICKS;
        list_add(tx_queue, &msg);
        msg.is_queued = true;
        old_status_flags = new_status_flags;

        // wait for message to be sent - note: it is possible to miss status changes
        // if the status changes and then changes back to old_status_flags while we
        // are waiting.
        PT_WAIT_WHILE(pt, msg.is_queued);
    }

    PT_END(pt);
}

PROCESS_THREAD(pbsys_bluetooth_process, ev, data) {
    static struct etimer timer;
    static struct pt status_monitor_pt;

    PROCESS_BEGIN();

    pbdrv_bluetooth_set_on_event(on_event);
    pbdrv_bluetooth_set_handle_rx(handle_rx);

    for (;;) {
        // make sure the Bluetooth chip is in reset long enough to actually reset
        etimer_set(&timer, clock_from_msec(150));
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        pbdrv_bluetooth_power_on(true);

        PROCESS_WAIT_UNTIL(pbdrv_bluetooth_is_ready());

        pbdrv_bluetooth_start_advertising();

        // TODO: allow user programs to initiate BLE connections
        pbsys_status_set(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);
        PROCESS_WAIT_UNTIL(pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_HCI) ||
            pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING));
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING);

        PT_INIT(&status_monitor_pt);

        for (;;) {
            if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_HCI)) {
                // disconnected
                break;
            }

            if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
                // Since pbsys status events are broadcast to all processes, this
                // will get triggered right away if there is a status change event.
                pbsys_bluetooth_monitor_status(&status_monitor_pt);
            }

            if (!tx_busy) {
                tx_msg_t *msg = list_pop(tx_queue);
                // msg->is_queued is set to false in tx_done callback rather than here
                if (msg) {
                    msg->context.done = tx_done;
                    if (msg->context.connection == PBDRV_BLUETOOTH_CONNECTION_UART) {
                        msg->context.size = lwrb_read(&uart_tx_ring, &msg->payload[0], PBIO_ARRAY_SIZE(msg->payload));
                        assert(msg->context.size);
                    }
                    msg->context.data = &msg->payload[0];
                    tx_busy = true;
                    pbdrv_bluetooth_tx(&msg->context);
                }
            }

            PROCESS_WAIT_EVENT();
        }

        reset_all();

        // reset Bluetooth chip
        pbdrv_bluetooth_power_on(false);
        PROCESS_WAIT_WHILE(pbdrv_bluetooth_is_ready());
        PROCESS_WAIT_WHILE(pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING));
    }

    PROCESS_END();
}

#endif // PBSYS_CONFIG_BLUETOOTH

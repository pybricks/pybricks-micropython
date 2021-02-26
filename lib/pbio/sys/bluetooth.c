// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_BLUETOOTH

#include <stdbool.h>
#include <stdint.h>

#include <contiki-lib.h>
#include <contiki.h>
#include <lwrb/lwrb.h>

#include <pbdrv/bluetooth.h>
#include <pbio/error.h>
#include <pbio/event.h>
#include <pbio/util.h>
#include <pbsys/status.h>
#include <pbsys/sys.h>

// REVISIT: this can be the negotiated MTU - 3 to allow for better throughput
// max data size for nRF UART characteristics
#define NRF_CHAR_SIZE 20

// nRF UART Rx hook
static pbsys_stdin_event_callback_t uart_rx_callback;
// nRF UART tx is in progress
static bool uart_tx_busy;
// buffer to queue UART tx data
static uint8_t uart_tx_buf[NRF_CHAR_SIZE];
// ring buffers for UART service
static lwrb_t uart_tx_ring;
static lwrb_t uart_rx_ring;

typedef struct {
    list_t queue;
    uint8_t payload[20];
    uint8_t size;
} pybricks_tx_msg_t;

MEMB(pybricks_tx_msg_pool, pybricks_tx_msg_t, 3);
LIST(pybricks_tx_queue);
static bool pybricks_tx_busy;

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
void pbsys_bluetooth_rx_set_callback(pbsys_stdin_event_callback_t callback) {
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
    // make sure we have a Bluetooth connection
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_UART)) {
        return PBIO_ERROR_INVALID_OP;
    }

    if ((*size = lwrb_write(&uart_tx_ring, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // poke the process to start tx soon-ish. This way, we can accumulate up to
    // NRF_CHAR_SIZE bytes before actually transmitting
    process_poll(&pbsys_bluetooth_process);

    return PBIO_SUCCESS;
}

static void uart_tx_done(const uint8_t *data) {
    uart_tx_busy = false;
    process_poll(&pbsys_bluetooth_process);
}

static void uart_on_rx(const uint8_t *data, uint8_t size) {
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

static void pybricks_tx_done(const uint8_t *data) {
    pybricks_tx_msg_t *msg = PBIO_CONTAINER_OF(data, pybricks_tx_msg_t, payload[0]);
    memb_free(&pybricks_tx_msg_pool, msg);
    pybricks_tx_busy = false;
    process_poll(&pbsys_bluetooth_process);
}

static void pybricks_on_rx(const uint8_t *data, uint8_t size) {
    // TODO: this is a temporary echo service - needs to trigger events instead

    pybricks_tx_msg_t *msg = memb_alloc(&pybricks_tx_msg_pool);
    if (!msg) {
        return;
    }

    msg->size = MIN(size, PBIO_ARRAY_SIZE(msg->payload));
    for (int i = 0; i < msg->size; i++) {
        msg->payload[i] = data[i] + 1;
    }

    list_add(pybricks_tx_queue, msg);

    process_poll(&pbsys_bluetooth_process);
}

PROCESS_THREAD(pbsys_bluetooth_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    pbdrv_bluetooth_set_on_event(on_event);
    pbdrv_bluetooth_uart_set_on_rx(uart_on_rx);
    pbdrv_bluetooth_pybricks_set_on_rx(pybricks_on_rx);

    for (;;) {
        // make sure the Bluetooth chip is in reset long enough to actually reset
        etimer_set(&timer, clock_from_msec(150));
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        pbdrv_bluetooth_power_on(true);

        PROCESS_WAIT_UNTIL(pbdrv_bluetooth_is_ready());

        pbdrv_bluetooth_start_advertising();

        // TODO: allow user programs to initiate BLE connections
        pbsys_status_set(PBSYS_STATUS_BLE_ADVERTISING);
        PROCESS_WAIT_UNTIL(pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_ANY) ||
            pbsys_status_test(PBSYS_STATUS_USER_PROGRAM_RUNNING));
        pbsys_status_clear(PBSYS_STATUS_BLE_ADVERTISING);

        for (;;) {
            if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_ANY)) {
                // disconnected
                break;
            }

            if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_UART) && !uart_tx_busy) {
                size_t size = lwrb_read(&uart_tx_ring, uart_tx_buf, NRF_CHAR_SIZE);
                if (size) {
                    uart_tx_busy = true;
                    pbdrv_bluetooth_uart_tx(uart_tx_buf, size, uart_tx_done);
                }
            }

            if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS) && !pybricks_tx_busy) {
                pybricks_tx_msg_t *msg = list_pop(pybricks_tx_queue);
                if (msg) {
                    pybricks_tx_busy = true;
                    pbdrv_bluetooth_pybricks_tx(&msg->payload[0], msg->size, pybricks_tx_done);
                }
            }

            PROCESS_WAIT_EVENT();
        }

        // reset Bluetooth chip
        pbdrv_bluetooth_power_on(false);
        PROCESS_WAIT_WHILE(pbdrv_bluetooth_is_ready());
        PROCESS_WAIT_WHILE(pbsys_status_test(PBSYS_STATUS_USER_PROGRAM_RUNNING));
    }

    PROCESS_END();
}

#endif // PBSYS_CONFIG_BLUETOOTH

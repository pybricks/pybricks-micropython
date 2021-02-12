// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_BLUETOOTH

#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/bluetooth.h>
#include <pbio/error.h>
#include <pbio/event.h>
#include <pbsys/status.h>
#include <pbsys/sys.h>

// REVISIT: this can be the negotiated MTU - 3 to allow for better throughput
// max data size for nRF UART characteristics
#define NRF_CHAR_SIZE 20

// nRF UART tx is in progress
static bool uart_tx_busy;
// buffer to queue UART tx data
static uint8_t uart_tx_buf[NRF_CHAR_SIZE];
// bytes used in uart_tx_buf
static uint8_t uart_tx_buf_size;

PROCESS(pbsys_bluetooth_process, "Bluetooth");

/** Initializes Bluetooth. */
void pbsys_bluetooth_init(void) {
    process_start(&pbsys_bluetooth_process, NULL);
}

static void on_event(void) {
    process_poll(&pbsys_bluetooth_process);
}

/**
 * Queues a character to be transmitted via Bluetooth serial port.
 * @param c [in]    the character to be sent.
 * @return          ::PBIO_SUCCESS if *c* was queued, ::PBIO_ERROR_AGAIN if the
 *                  character could not be queued at this time (e.g. buffer is
 *                  full), ::PBIO_ERROR_INVALID_OP if there is not an active
 *                  Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED if this
 *                  platform does not support Bluetooth.
 */
pbio_error_t pbsys_bluetooth_tx(uint8_t c) {
    // make sure we have a Bluetooth connection
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_UART)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // can't add the the buffer if tx is in progress or buffer is full
    if (uart_tx_busy || uart_tx_buf_size == NRF_CHAR_SIZE) {
        return PBIO_ERROR_AGAIN;
    }

    // append the char
    uart_tx_buf[uart_tx_buf_size] = c;
    uart_tx_buf_size++;

    // poke the process to start tx soon-ish. This way, we can accumulate up to
    // NRF_CHAR_SIZE bytes before actually transmitting
    process_poll(&pbsys_bluetooth_process);

    return PBIO_SUCCESS;
}

static void uart_tx_done(void) {
    uart_tx_busy = false;
    uart_tx_buf_size = 0;
    process_poll(&pbsys_bluetooth_process);
}

static void uart_on_rx(const uint8_t *data, uint8_t size) {
    for (int i = 0; i < size; i++) {
        pbsys_stdin_put_char(data[i]);
    }
}

static void pybricks_tx_done(void) {
    process_poll(&pbsys_bluetooth_process);
}

static void pybricks_on_rx(const uint8_t *data, uint8_t size) {
    // TODO: this is a temporary echo service - needs to trigger events instead
    static uint8_t copy[NRF_CHAR_SIZE];

    for (int i = 0; i < MIN(size, NRF_CHAR_SIZE); i++) {
        copy[i] = data[i] + 1;
    }

    pbdrv_bluetooth_pybricks_tx(copy, size, pybricks_tx_done);
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

        PROCESS_WAIT_WHILE(!pbdrv_bluetooth_is_ready());

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

            if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_UART) && !uart_tx_busy && uart_tx_buf_size) {
                uart_tx_busy = true;
                pbdrv_bluetooth_uart_tx(uart_tx_buf, uart_tx_buf_size, uart_tx_done);
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

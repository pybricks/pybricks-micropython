// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 UART driver for BlueKitchen BTStack (stubs).

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3_UART

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <btstack.h>
#include <btstack_uart_block.h>
#include <pbdrv/uart.h>
#include <pbio/os.h>

#include "bluetooth_btstack_uart_block_ev3.h"

// If a read has been requested, a pointer to the buffer and its length, else
// null and zero.
static uint8_t *read_buf;
static int read_buf_len;

// If a write has been requested, a pointer to the buffer and its length, else
// null and zero.
static const uint8_t *write_buf;
static int write_buf_len;

// Should the reader and writer processes shut down?
static bool start_shutdown;

// Count of threads that have finished since start_shutdown was set.
static int8_t threads_shutdown_complete;

// Called when a block finishes sending.
static void (*block_sent)(void);
// Called when a block finishes being received.
static void (*block_received)(void);

// Processes for reading and writing blocks.
static pbio_os_process_t reader_process;
static pbio_os_process_t writer_process;

static pbdrv_uart_dev_t *pbdrv_bluetooth_btstack_uart_block_ev3_uart_device() {
    const pbdrv_bluetooth_btstack_uart_block_ev3_platform_data_t *pdata =
        &pbdrv_bluetooth_btstack_uart_block_ev3_platform_data;
    pbdrv_uart_dev_t *uart;
    pbio_error_t err = pbdrv_uart_get_instance(pdata->uart_id, &uart);
    (void)err;
    assert(err == PBIO_SUCCESS);
    return uart;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_init(const btstack_uart_config_t *config) {
    pbdrv_uart_set_baud_rate(pbdrv_bluetooth_btstack_uart_block_ev3_uart_device(), config->baudrate);
    // TODO: add parity, flow control APIs and obey them.

    return 0;
}

static pbio_error_t pbdrv_bluetooth_btstack_uart_block_ev3_do_read_process(pbio_os_state_t *state, void *context) {
    pbdrv_uart_dev_t *const uart = pbdrv_bluetooth_btstack_uart_block_ev3_uart_device();

    pbio_os_state_t read_state;
    PBIO_OS_ASYNC_BEGIN(state);

    while (true) {
        PBIO_OS_AWAIT_UNTIL(state, read_buf || start_shutdown);
        if (start_shutdown) {
            break;
        }

        PBIO_OS_AWAIT(state, &read_state,
            pbdrv_uart_read(state, uart, read_buf, read_buf_len, /*timeout=*/ 0));
        read_buf = NULL;
        read_buf_len = 0;
        if (block_received) {
            block_received();
        }
    }

    ++threads_shutdown_complete;
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_error_t pbdrv_bluetooth_btstack_uart_block_ev3_do_write_process(pbio_os_state_t *state, void *context) {
    pbdrv_uart_dev_t *const uart = pbdrv_bluetooth_btstack_uart_block_ev3_uart_device();

    pbio_os_state_t write_state;

    PBIO_OS_ASYNC_BEGIN(state);

    while (true) {
        PBIO_OS_AWAIT_UNTIL(state, write_buf || start_shutdown);
        if (start_shutdown) {
            break;
        }

        PBIO_OS_AWAIT(state, &write_state,
            pbdrv_uart_write(&write_state, uart, (uint8_t *)write_buf, write_buf_len, /*timeout=*/ 0));
        write_buf = NULL;
        write_buf_len = 0;
        if (block_sent) {
            block_sent();
        }
    }

    ++threads_shutdown_complete;
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}


static int pbdrv_bluetooth_btstack_uart_block_ev3_open(void) {
    write_buf = NULL;
    write_buf_len = 0;
    read_buf = NULL;
    read_buf_len = 0;
    start_shutdown = false;
    threads_shutdown_complete = 0;
    block_received = NULL;
    block_sent = NULL;

    pbio_os_process_start(&reader_process, pbdrv_bluetooth_btstack_uart_block_ev3_do_read_process, NULL);
    pbio_os_process_start(&writer_process, pbdrv_bluetooth_btstack_uart_block_ev3_do_write_process, NULL);

    return 0;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_close(void) {
    start_shutdown = true;
    while (threads_shutdown_complete < 2) {
        pbio_os_run_processes_and_wait_for_event();
    }

    return 0;
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_set_block_received(void (*handler)(void)) {
    block_received = handler;
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_set_block_sent(void (*handler)(void)) {
    block_sent = handler;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_set_baudrate(uint32_t baud) {
    pbdrv_uart_set_baud_rate(pbdrv_bluetooth_btstack_uart_block_ev3_uart_device(), baud);
    return 0;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_set_parity(int parity) {
    // TODO: maybe implement the parity setting.
    return 0;
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_receive_block(uint8_t *buffer,
    uint16_t len) {
    read_buf = buffer;
    read_buf_len = len;
    pbio_os_request_poll();
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_send_block(const uint8_t *data,
    uint16_t size) {
    write_buf = data;
    write_buf_len = size;
    pbio_os_request_poll();
}

static const btstack_uart_block_t pbdrv_bluetooth_btstack_uart_block_ev3_block_ev3 = {
    .init = pbdrv_bluetooth_btstack_uart_block_ev3_init,
    .open = pbdrv_bluetooth_btstack_uart_block_ev3_open,
    .close = pbdrv_bluetooth_btstack_uart_block_ev3_close,
    .set_block_received = pbdrv_bluetooth_btstack_uart_block_ev3_set_block_received,
    .set_block_sent = pbdrv_bluetooth_btstack_uart_block_ev3_set_block_sent,
    .set_baudrate = pbdrv_bluetooth_btstack_uart_block_ev3_set_baudrate,
    .set_parity = pbdrv_bluetooth_btstack_uart_block_ev3_set_parity,
    .set_flowcontrol = NULL,
    .receive_block = pbdrv_bluetooth_btstack_uart_block_ev3_receive_block,
    .send_block = pbdrv_bluetooth_btstack_uart_block_ev3_send_block,
    .get_supported_sleep_modes = NULL,
    .set_sleep = NULL,
    .set_wakeup_handler = NULL,
};

const btstack_uart_block_t *pbdrv_bluetooth_btstack_uart_block_ev3_instance(
    void) {
    return &pbdrv_bluetooth_btstack_uart_block_ev3_block_ev3;
}

#endif  // PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3_UART

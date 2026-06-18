// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Temporary hack to get stdio over UART on Build HAT

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_SIMULATION_PICO

#include <string.h>

#include <lwrb/lwrb.h>

#include <pbdrv/usb.h>
#include <pbio/error.h>
#include <pbio/int_math.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "usb.h"

static lwrb_t pbdrv_usb_simulation_pico_in_ringbuf;
static volatile bool pbdrv_usb_simulation_tx_ready;

// Size of the UART receive ring buffer. This is a mock with no real USB
// hardware, so the value is arbitrary.
#define PBDRV_USB_SIMULATION_PICO_RX_RINGBUF_SIZE (128)

pbio_error_t pbdrv_usb_wait_until_configured(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

bool pbdrv_usb_is_ready(void) {
    return true;
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

pbio_error_t pbdrv_usb_tx_chunk(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {

    PBIO_OS_ASYNC_BEGIN(state);

    // The common driver hands us a COBS-encoded frame with a trailing
    // delimiter. This mock only forwards stdout to the UART, so decode the
    // frame and ignore anything that is not a stdout event.
    // REVISIT: This assumes that we do one chunk per stdout event. That is
    // currently true for the logic in usb.c, but we should revise this to make
    // it like the RX path if we turn it into an actual stream.
    static uint8_t msg[PBDRV_USB_MAX_DECODED_MESSAGE_SIZE];
    static uint32_t msg_size;
    msg_size = pbdrv_usb_cobs_decode(data, size - 1, msg, sizeof(msg));

    if (msg_size < 2 || msg[0] != PBIO_PYBRICKS_IN_EP_MSG_EVENT ||
        msg[1] != PBIO_PYBRICKS_EVENT_WRITE_STDOUT) {
        return PBIO_SUCCESS;
    }

    static int i;
    for (i = 2; i < msg_size; i++) {
        if (!uart_is_writable(uart_default)) {
            pbdrv_usb_simulation_tx_ready = false;
            // Enable TX interrupt to be notified when ready.
            hw_set_bits(&uart_get_hw(uart_default)->imsc, 1 << UART_UARTIMSC_TXIM_LSB);
            PBIO_OS_AWAIT_UNTIL(state, pbdrv_usb_simulation_tx_ready);
        }

        uart_get_hw(uart_default)->dr = msg[i];
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    return PBIO_SUCCESS;
}

static uint8_t pbdrv_usb_simulation_pico_in_buf[PBDRV_USB_MAX_ENCODED_MESSAGE_SIZE];
static uint32_t pbdrv_usb_simulation_pico_in_size;

uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {
    uint32_t size = pbdrv_usb_simulation_pico_in_size;

    // Invalid size.
    if (size > sizeof(pbdrv_usb_simulation_pico_in_buf)) {
        pbdrv_usb_simulation_pico_in_size = 0;
        return 0;
    }

    // Nothing received yet.
    if (size == 0) {
        return 0;
    }

    memcpy(data, pbdrv_usb_simulation_pico_in_buf, size);

    // Reset to indicate we wait for new data.
    pbdrv_usb_simulation_pico_in_size = 0;

    return size;
}

// Simulates incoming USB data by reading from native host stdin. In
// MicroPython, it drives the REPL.
static pbio_error_t pbdrv_usb_test_process_thread(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {
        static size_t available;
        PBIO_OS_AWAIT_UNTIL(state, pbdrv_usb_simulation_pico_in_size == 0 &&
            (available = lwrb_get_full(&pbdrv_usb_simulation_pico_in_ringbuf)) > 0);

        available = pbio_int_math_clamp(available, PBDRV_USB_MAX_DECODED_MESSAGE_SIZE - 2);

        // Wrap the raw UART bytes as a write stdin command and COBS-encode it,
        // the same way a real host would, so the common driver can decode it.
        static uint8_t cmd[PBDRV_USB_MAX_DECODED_MESSAGE_SIZE];
        cmd[0] = PBIO_PYBRICKS_OUT_EP_MSG_COMMAND;
        cmd[1] = PBIO_PYBRICKS_COMMAND_WRITE_STDIN;
        lwrb_read(&pbdrv_usb_simulation_pico_in_ringbuf, &cmd[2], available);
        pbdrv_usb_simulation_pico_in_size = pbdrv_usb_cobs_encode(
            cmd, 2 + available, pbdrv_usb_simulation_pico_in_buf);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static void pbdrv_usb_simulation_pico_uart_rx_irq(void) {
    // Clear all interrupts.
    uart_get_hw(uart_default)->icr = UART_UARTICR_BITS;

    if (uart_is_writable(uart_default)) {
        pbdrv_usb_simulation_tx_ready = true;
        // We don't write to dr here, so disable TX interrupt to avoid retriggering.
        hw_clear_bits(&uart_get_hw(uart_default)->imsc, 1 << UART_UARTIMSC_TXIM_LSB);
    }

    while (uart_is_readable(uart_default)) {
        uint8_t byte = uart_get_hw(uart_default)->dr;
        lwrb_write(&pbdrv_usb_simulation_pico_in_ringbuf, &byte, 1);
    }

    pbio_os_request_poll();
}

void pbdrv_usb_init_device(void) {
    static uint8_t in_ringbuf_data[PBDRV_USB_SIMULATION_PICO_RX_RINGBUF_SIZE];
    lwrb_init(&pbdrv_usb_simulation_pico_in_ringbuf, in_ringbuf_data, sizeof(in_ringbuf_data));

    setup_default_uart();
    irq_set_exclusive_handler(UART_IRQ_NUM(uart_default), pbdrv_usb_simulation_pico_uart_rx_irq);
    irq_set_enabled(UART_IRQ_NUM(uart_default), true);
    uart_set_irqs_enabled(uart_default, true, true);

    static pbio_os_process_t pbdrv_usb_test_process;
    pbio_os_process_start(&pbdrv_usb_test_process, pbdrv_usb_test_process_thread, NULL);

    // No physical port to detect, so report the host connection as active
    // right away (USB analog of a BLE host subscribing).
    pbdrv_usb_on_dtr_changed(true);
}

void pbdrv_usb_deinit_device(void) {
}

#endif // PBDRV_CONFIG_USB_SIMULATION_PICO

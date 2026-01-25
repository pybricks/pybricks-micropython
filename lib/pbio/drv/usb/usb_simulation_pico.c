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

pbio_error_t pbdrv_usb_wait_for_charger(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

bool pbdrv_usb_is_ready(void) {
    return true;
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

pbio_error_t pbdrv_usb_tx_event(pbio_os_state_t *state, const uint8_t *data, uint32_t size, bool cancel) {

    PBIO_OS_ASYNC_BEGIN(state);

    // Only care about stdout for now.
    if (size < 2 || data[0] != PBIO_PYBRICKS_IN_EP_MSG_EVENT ||
        data[1] != PBIO_PYBRICKS_EVENT_WRITE_STDOUT) {
        return PBIO_SUCCESS;
    }

    static int i;
    for (i = 2; i < size; i++) {
        if (!uart_is_writable(uart_default)) {
            pbdrv_usb_simulation_tx_ready = false;
            // Enable TX interrupt to be notified when ready.
            hw_set_bits(&uart_get_hw(uart_default)->imsc, 1 << UART_UARTIMSC_TXIM_LSB);
            PBIO_OS_AWAIT_UNTIL(state, pbdrv_usb_simulation_tx_ready || cancel);
            if (!pbdrv_usb_simulation_tx_ready && cancel) {
                return PBIO_ERROR_CANCELED;
            }
        }

        uart_get_hw(uart_default)->dr = data[i];
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_response(pbio_os_state_t *state, pbio_pybricks_error_t code, bool cancel) {

    PBIO_OS_ASYNC_BEGIN(state);

    // Not implemented.

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    return PBIO_SUCCESS;
}

static uint8_t pbdrv_usb_simulation_pico_in_buf[PBDRV_CONFIG_USB_MAX_PACKET_SIZE];
static uint32_t pbdrv_usb_simulation_pico_in_size;

uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {
    uint32_t size = pbdrv_usb_simulation_pico_in_size;

    // Invalid size.
    if (size > PBDRV_CONFIG_USB_MAX_PACKET_SIZE) {
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

    // Simulate subscribe event.
    pbdrv_usb_simulation_pico_in_buf[0] = PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE;
    pbdrv_usb_simulation_pico_in_buf[1] = 1;
    pbdrv_usb_simulation_pico_in_size = 2;

    for (;;) {
        static size_t available;
        PBIO_OS_AWAIT_UNTIL(state, pbdrv_usb_simulation_pico_in_size == 0 &&
            (available = lwrb_get_full(&pbdrv_usb_simulation_pico_in_ringbuf)) > 0);

        available = pbio_int_math_clamp(available, PBDRV_CONFIG_USB_MAX_PACKET_SIZE - 2);

        pbdrv_usb_simulation_pico_in_buf[0] = PBIO_PYBRICKS_OUT_EP_MSG_COMMAND;
        pbdrv_usb_simulation_pico_in_buf[1] = PBIO_PYBRICKS_COMMAND_WRITE_STDIN;
        lwrb_read(&pbdrv_usb_simulation_pico_in_ringbuf,
            &pbdrv_usb_simulation_pico_in_buf[2], available);
        pbdrv_usb_simulation_pico_in_size = 2 + available;
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
    static uint8_t in_ringbuf_data[PBDRV_CONFIG_USB_MAX_PACKET_SIZE * 2];
    lwrb_init(&pbdrv_usb_simulation_pico_in_ringbuf, in_ringbuf_data, sizeof(in_ringbuf_data));

    setup_default_uart();
    irq_set_exclusive_handler(UART_IRQ_NUM(uart_default), pbdrv_usb_simulation_pico_uart_rx_irq);
    irq_set_enabled(UART_IRQ_NUM(uart_default), true);
    uart_set_irqs_enabled(uart_default, true, true);

    static pbio_os_process_t pbdrv_usb_test_process;
    pbio_os_process_start(&pbdrv_usb_test_process, pbdrv_usb_test_process_thread, NULL);
}

void pbdrv_usb_deinit_device(void) {
}

#endif // PBDRV_CONFIG_USB_SIMULATION_PICO

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_DEBUG_UART

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <contiki.h>

#include <pbdrv/uart.h>
#include <pbdrv/ioport.h>

#include "ioport_pup.h"

#if (PBDRV_CONFIG_IOPORT_NUM_DEV - PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV) != 1
#error "PBDRV_CONFIG_IOPORT_DEBUG_UART requires exactly one port"
#endif

static char buffer[64];
static pbdrv_uart_dev_t *debug_uart;

#define UART_TIMEOUT (250)

static pbio_error_t pbdrv_ioport_init(void) {
    // UART already initialized.
    if (debug_uart) {
        return PBIO_SUCCESS;
    }

    // Get the debug uart from last port.
    const pbdrv_ioport_pup_port_platform_data_t *port_data = &pbdrv_ioport_pup_platform_data.ports[PBDRV_CONFIG_IOPORT_NUM_DEV - 1];
    pbio_error_t err = pbdrv_uart_get(port_data->uart_driver_index, &debug_uart);
    if (err != PBIO_SUCCESS) {
        debug_uart = NULL;
        return err;
    }

    // Enable and configure uart.
    const pbdrv_ioport_pup_pins_t *pins = &port_data->pins;
    pbdrv_gpio_alt(&pins->uart_rx, pins->uart_alt);
    pbdrv_gpio_alt(&pins->uart_tx, pins->uart_alt);
    pbdrv_gpio_out_low(&pins->uart_buf);

    pbdrv_uart_set_baud_rate(debug_uart, 115200);
    pbdrv_uart_flush(debug_uart);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_ioport_debug_uart_printf(const char *format, ...) {

    // Get the debug uart.
    pbio_error_t err = pbdrv_ioport_init();
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If still writing, just ignore this debug message.
    err = pbdrv_uart_write_end(debug_uart);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Buffer result.
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Write to uart.
    return pbdrv_uart_write_begin(debug_uart, (uint8_t *)buffer, strlen(buffer), UART_TIMEOUT);
}

// Protothread set up here for simplified debug macro inside protothreads.
struct pt printf_thread;

PT_THREAD(pbdrv_ioport_debug_uart_printf_thread(struct pt *pt, const char *format, ...)) {
    static pbio_error_t err;

    PT_BEGIN(pt);

    // Only print if port initialized.
    err = pbdrv_ioport_init();
    if (err != PBIO_SUCCESS) {
        PT_EXIT(pt);
    }

    // Buffer result.
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Wait for uart to be ready then write.
    PT_WAIT_UNTIL((pt), (err = pbdrv_uart_write_begin(debug_uart, (uint8_t *)buffer, strlen(buffer), UART_TIMEOUT)) != PBIO_ERROR_AGAIN);
    if (err != PBIO_SUCCESS) {
        PT_EXIT(pt);
    }

    // Wait for uart to finish.
    PT_WAIT_UNTIL((pt), (err = pbdrv_uart_write_end(debug_uart)) != PBIO_ERROR_AGAIN);

    PT_END(pt);
}

#endif // PBDRV_CONFIG_IOPORT_DEBUG_UART

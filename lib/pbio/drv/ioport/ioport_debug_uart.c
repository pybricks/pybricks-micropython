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

// Use globals for simplified debug macro inside protothreads.
struct pt debug_printf_thread;
char debug_buffer[64];
pbdrv_uart_dev_t *debug_uart;
pbio_error_t debug_err;

pbio_error_t pbdrv_ioport_debug_uart_init(pbdrv_uart_poll_callback_t callback) {

    // Get the debug uart from last port.
    const pbdrv_ioport_pup_port_platform_data_t *port_data = &pbdrv_ioport_pup_platform_data.ports[PBDRV_CONFIG_IOPORT_NUM_DEV - 1];
    pbio_error_t err = pbdrv_uart_get(port_data->uart_driver_index, &debug_uart);
    if (err != PBIO_SUCCESS) {
        debug_uart = NULL;
        return err;
    }

    pbdrv_uart_set_poll_callback(debug_uart, callback);

    // Enable and configure uart.
    const pbdrv_ioport_pup_pins_t *pins = &port_data->pins;
    pbdrv_gpio_alt(&pins->uart_rx, pins->uart_alt);
    pbdrv_gpio_alt(&pins->uart_tx, pins->uart_alt);
    pbdrv_gpio_out_low(&pins->uart_buf);

    pbdrv_uart_set_baud_rate(debug_uart, 115200);
    pbdrv_uart_flush(debug_uart);
    return PBIO_SUCCESS;
}

void pbdrv_ioport_debug_uart_printf_buffer(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(debug_buffer, sizeof(debug_buffer), format, args);
    va_end(args);
}

#endif // PBDRV_CONFIG_IOPORT_DEBUG_UART

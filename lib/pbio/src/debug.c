// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2025 The Pybricks Authors

#include <pbdrv/usb.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>

#include <pbio/debug.h>

void pbio_debug_va(const char *format, va_list args) {
    char buf[256];
    size_t len = vsnprintf(buf, sizeof(buf), format, args);
    pbdrv_usb_debug_print(buf, len);
    pbdrv_uart_debug_print(buf, len);
}

void pbio_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    pbio_debug_va(format, args);
    va_end(args);
}

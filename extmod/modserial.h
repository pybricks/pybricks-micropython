// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/error.h>
#include <unistd.h>
#include <stdint.h>

typedef struct _serial_t serial_t;

#define UART_MAX_LEN (32*1024)

pbio_error_t serial_get(serial_t **_ser, int tty, int baudrate, int timeout);

pbio_error_t serial_write(serial_t *ser, const void *buf, size_t count);

pbio_error_t serial_in_waiting(serial_t *ser, size_t *waiting);

pbio_error_t serial_read_blocking(serial_t *ser, uint8_t *buf, size_t count, size_t *remaining, int32_t time_start, int32_t time_now);

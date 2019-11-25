// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/error.h>
#include <unistd.h>
#include <stdint.h>

#include <pbio/port.h>
#include <pbdrv/serial.h>

typedef struct _pbio_serial_t {
    pbdrv_serial_t *dev;
} pbio_serial_t;

pbio_error_t pbio_serial_get(pbio_serial_t **_ser, pbio_port_t port, int baudrate, int timeout);

pbio_error_t pbio_serial_write(pbio_serial_t *ser, const void *buf, size_t count);

pbio_error_t pbio_serial_in_waiting(pbio_serial_t *ser, size_t *waiting);

pbio_error_t pbio_serial_read_blocking(pbio_serial_t *ser, uint8_t *buf, size_t count, size_t *remaining, int32_t time_start, int32_t time_now);

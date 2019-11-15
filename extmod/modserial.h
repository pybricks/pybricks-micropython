// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/error.h>
#include <unistd.h>

typedef struct _serial_t serial_t;

pbio_error_t serial_get(serial_t **_ser, int tty, int baudrate, int timeout);

pbio_error_t serial_write(serial_t *ser, const void *buf, size_t count);

pbio_error_t serial_in_waiting(serial_t *ser, size_t *waiting);

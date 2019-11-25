// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <pbio/error.h>
#include <pbio/serial.h>

pbio_error_t pbio_serial_get(pbio_serial_t **_ser, int tty, int baudrate, int timeout) {
    return pbdrv_serial_get(_ser, tty, baudrate, timeout);
}

pbio_error_t pbio_serial_write(pbio_serial_t *ser, const void *buf, size_t count) {
    return pbdrv_serial_write(ser, buf, count);
}

pbio_error_t pbio_serial_in_waiting(pbio_serial_t *ser, size_t *waiting) {
    return pbdrv_serial_in_waiting(ser, waiting);
}

pbio_error_t pbio_serial_read_blocking(pbio_serial_t *ser, uint8_t *buf, size_t count, size_t *remaining, int32_t time_start, int32_t time_now) {
    return pbdrv_serial_read_blocking(ser, buf, count, remaining, time_start, time_now);
}

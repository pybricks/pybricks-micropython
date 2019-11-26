// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <pbio/port.h>
#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/serial.h>

pbio_serial_t serials[PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT - PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT + 1];

pbio_error_t pbio_serial_get(pbio_serial_t **_ser, pbio_port_t port, int baudrate, int timeout) {

    if (port < PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT || port > PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbio_serial_t *ser = &serials[port-PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT];

    pbio_error_t err = pbdrv_serial_get(&ser->dev, port, baudrate);

    if (err != PBIO_SUCCESS) {
        return err;
    }

    ser->timeout = timeout >= 0 ? timeout : -1;

    *_ser = ser;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_serial_write(pbio_serial_t *ser, const void *buf, size_t count) {
    return pbdrv_serial_write(ser->dev, buf, count);
}

pbio_error_t pbio_serial_in_waiting(pbio_serial_t *ser, size_t *waiting) {
    return pbdrv_serial_in_waiting(ser->dev, waiting);
}

pbio_error_t pbio_serial_read(pbio_serial_t *ser, uint8_t *buf, size_t count, size_t *remaining, int32_t time_start, int32_t time_now) {

    pbio_error_t err;

    // Read and keep track of how much was read
    size_t read_now;
    err = pbdrv_serial_read(ser->dev, &buf[count - *remaining], count, &read_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Decrement remaining count
    *remaining -= read_now;

    // If there is nothing remaining, we are done
    if (*remaining == 0) {
        return PBIO_SUCCESS;
    }

    // If we have timed out, let the user know
    if (ser->timeout >= 0 && time_now - time_start > ser->timeout) {
        return PBIO_ERROR_TIMEDOUT;
    }

    // If we are here, we need to call this again
    return PBIO_ERROR_AGAIN;
}

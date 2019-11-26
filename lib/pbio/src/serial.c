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

#include "sys/clock.h"

struct _pbio_serial_t {
    pbdrv_serial_t *dev;
    int timeout;
    bool busy;
    int time_start;
    size_t remaining;
};

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

static pbio_error_t pbio_serial_read_start(pbio_serial_t *ser, size_t count) {
    // Already started, so return
    if (ser->busy) {
        return PBIO_SUCCESS;
    }

    // Reset state variables
    ser->busy = true;
    ser->time_start = clock_usecs()/1000;
    ser->remaining = count;

    return PBIO_SUCCESS;
}

static pbio_error_t pbio_serial_read_stop(pbio_serial_t *ser, pbio_error_t err) {
    // Return to default state
    ser->busy = false;

    // Return the error that was raised on stopping
    return err;
}

pbio_error_t pbio_serial_read(pbio_serial_t *ser, uint8_t *buf, size_t count) {

    pbio_error_t err;

    // If this is called for the first time, init:
    err = pbio_serial_read_start(ser, count);
    if (err != PBIO_SUCCESS) {
        return pbio_serial_read_stop(ser, err);
    }

    // Read and keep track of how much was read
    size_t read_now;
    err = pbdrv_serial_read(ser->dev, &buf[count - ser->remaining], count, &read_now);
    if (err != PBIO_SUCCESS) {
        return pbio_serial_read_stop(ser, err);
    }

    // Decrement remaining count
    ser->remaining -= read_now;

    // If there is nothing remaining, we are done
    if (ser->remaining == 0) {
        return pbio_serial_read_stop(ser, PBIO_SUCCESS);
    }

    // If we have timed out, let the user know
    if (ser->timeout >= 0 && clock_usecs()/1000 - ser->time_start > ser->timeout) {
        return pbio_serial_read_stop(ser, PBIO_ERROR_TIMEDOUT);
    }

    // If we are here, we need to call this again
    return PBIO_ERROR_AGAIN;
}

pbio_error_t pbio_serial_clear(pbio_serial_t *ser) {
    return pbdrv_serial_clear(ser->dev);
}

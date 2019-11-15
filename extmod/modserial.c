// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <fcntl.h>
#include <termios.h>

#include <pbio/error.h>

#include "modserial.h"

static const char* const TTY_PATH[] = {
    "/dev/tty_ev3-ports:in1",
    "/dev/tty_ev3-ports:in2",
    "/dev/tty_ev3-ports:in3",
    "/dev/tty_ev3-ports:in4",
};

struct _serial_t {
    int file;
    int baudrate;
    int timeout;
};

serial_t serials[sizeof(TTY_PATH)/sizeof(TTY_PATH[0])];
const int num_tty = sizeof(TTY_PATH)/sizeof(TTY_PATH[0]);

static pbio_error_t serial_open(serial_t *ser, int tty) {

    if (tty < 0 || tty >= num_tty) {
        return PBIO_ERROR_INVALID_PORT;
    }

    ser->file = open(TTY_PATH[tty], O_RDWR | O_NOCTTY);
    if (ser->file == -1) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}


pbio_error_t serial_get(serial_t **_ser, int tty, int baudrate, int timeout) {

    // Get device pointer
    pbio_error_t err;
    serial_t *ser = &serials[tty];

    // Configure settings
    switch (baudrate) {
        case 9600:
            ser->baudrate = B9600;
            break;
        case 115200:
            ser->baudrate = B115200;
            break;        
        default:
            return PBIO_ERROR_INVALID_ARG;
    }
    ser->timeout = timeout;

    // Open serial port
    err = serial_open(ser, tty);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return pointer to device
    *_ser = ser;

    return PBIO_SUCCESS;
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

// Portions of this file (termios settings) adapted from MicroPython, modtermios.c
// Copyright (c) 2014-2015 Paul Sokolovsky

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

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

static pbio_error_t serial_config(serial_t *ser, int baudrate) {

    // Convert to termios baudrate
    speed_t speed;
    switch (baudrate) {
        case 9600:
            speed = B9600;
            break;
        case 115200:
            speed = B115200;
            break;        
        default:
            return PBIO_ERROR_INVALID_ARG;
    }

    // Get termios attributes
    struct termios term;
    if (tcgetattr(ser->file, &term) != 0) {
        return PBIO_ERROR_IO;
    }

    // Set termios attributes
    term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term.c_oflag = 0;
    term.c_cflag = (term.c_cflag & ~(CSIZE | PARENB)) | CS8;
    term.c_lflag = 0;
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    if (cfsetispeed(&term, speed) != 0) {
        return PBIO_ERROR_IO;
    }

    if (cfsetospeed(&term, speed) != 0) {
        return PBIO_ERROR_IO;
    }

    // Write attributes
    if (tcsetattr(ser->file, TCSAFLUSH, &term) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}


pbio_error_t serial_get(serial_t **_ser, int tty, int baudrate, int timeout) {

    // Get device pointer
    pbio_error_t err;
    serial_t *ser = &serials[tty];

    // Configure settings
    ser->timeout = timeout;

    // Open serial port
    err = serial_open(ser, tty);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Config serial port
    err = serial_config(ser, baudrate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return pointer to device
    *_ser = ser;

    return PBIO_SUCCESS;
}

pbio_error_t serial_write(serial_t *ser, const void *buf, size_t count) {
    if (write(ser->file, buf, count) != count) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

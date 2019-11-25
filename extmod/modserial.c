// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

// Portions of this file (termios settings) adapted from MicroPython, modtermios.c
// Copyright (c) 2014-2015 Paul Sokolovsky

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
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

    ser->file = open(TTY_PATH[tty], O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (ser->file == -1) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

static pbio_error_t serial_config(serial_t *ser, int baudrate) {

    // Convert to termios baudrate
    speed_t speed;
    switch (baudrate) {
        case 50: speed = B50; break;
        case 75: speed = B75; break;
        case 110: speed = B110; break;
        case 134: speed = B134; break;
        case 150: speed = B150; break;
        case 200: speed = B200; break;
        case 300: speed = B300; break;
        case 600: speed = B600; break;
        case 1200: speed = B1200; break;
        case 1800: speed = B1800; break;
        case 2400: speed = B2400; break;
        case 4800: speed = B4800; break;
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 500000: speed = B500000; break;
        case 576000: speed = B576000; break;
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

pbio_error_t serial_in_waiting(serial_t *ser, size_t *waiting) {
    if (ioctl(ser->file, FIONREAD, waiting) == -1) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

static pbio_error_t serial_read_count(serial_t *ser, uint8_t *buf, size_t count, size_t *result) {
    int ret = read(ser->file, buf, count);
    if (ret < 0) {
        if (errno == EAGAIN) {
            *result = 0;
            return PBIO_SUCCESS;
        }
        else {
            return PBIO_ERROR_IO;
        }
    }
    *result = ret;
    return PBIO_SUCCESS;
}

pbio_error_t serial_read_blocking(serial_t *ser, uint8_t *buf, size_t count, size_t *remaining, int32_t time_start, int32_t time_now) {

    pbio_error_t err;

    // Read and keep track of how much was read
    size_t read_now;
    err = serial_read_count(ser, &buf[count - *remaining], count, &read_now);
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

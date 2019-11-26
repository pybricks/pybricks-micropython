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

#include <pbdrv/serial.h>

static const char* const TTY_PATH[] = {
    "/dev/tty_ev3-ports:in1",
    "/dev/tty_ev3-ports:in2",
    "/dev/tty_ev3-ports:in3",
    "/dev/tty_ev3-ports:in4",
};

struct _pbdrv_serial_t {
    int file;
    int timeout;
};

pbdrv_serial_t pbdrv_serials[sizeof(TTY_PATH)/sizeof(TTY_PATH[0])];

static pbio_error_t pbdrv_serial_open(pbdrv_serial_t *ser, const char *path) {

    ser->file = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (ser->file == -1) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_serial_config(pbdrv_serial_t *ser, int baudrate) {

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

    if (cfsetspeed(&term, speed) != 0) {
        return PBIO_ERROR_IO;
    }

    // Write attributes
    if (tcsetattr(ser->file, TCSAFLUSH, &term) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}


pbio_error_t pbdrv_serial_get(pbdrv_serial_t **_ser, pbio_port_t port, int baudrate) {

    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get device pointer
    pbio_error_t err;
    pbdrv_serial_t *ser = &pbdrv_serials[port-PBIO_PORT_1];

    // Open pbdrv_serial port
    err = pbdrv_serial_open(ser, TTY_PATH[port-PBIO_PORT_1]);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Config pbdrv_serial port
    err = pbdrv_serial_config(ser, baudrate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return pointer to device
    *_ser = ser;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_serial_write(pbdrv_serial_t *ser, const void *buf, size_t count) {
    if (write(ser->file, buf, count) != count) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_serial_in_waiting(pbdrv_serial_t *ser, size_t *waiting) {
    if (ioctl(ser->file, FIONREAD, waiting) == -1) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_serial_read(pbdrv_serial_t *ser, uint8_t *buf, size_t count, size_t *received) {
    int ret = read(ser->file, buf, count);
    if (ret < 0) {
        if (errno == EAGAIN) {
            *received = 0;
            return PBIO_SUCCESS;
        }
        else {
            return PBIO_ERROR_IO;
        }
    }
    *received = ret;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_serial_clear(pbdrv_serial_t *ser) {
    if (tcflush(ser->file, TCIOFLUSH) != 0) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

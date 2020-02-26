// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <ev3dev_stretch/lego_sensor.h>
#include <ev3dev_stretch/sysfs.h>

#include <pbio/iodev.h>
#include <pbio/port.h>
#include <pbio/util.h>

#define MAX_PATH_LENGTH 60
#define MAX_READ_LENGTH "60"
#define BIN_DATA_SIZE   32 // size of bin_data sysfs attribute

typedef enum {
    NO_MOTOR,
    NO_SENSOR,
    AUTO,
    NXT_ANALOG,
    NXT_COLOR,
    NXT_I2C,
    OTHER_I2C,
    EV3_ANALOG,
    EV3_UART,
    OTHER_UART,
    RAW,
    TACHO_MOTOR,
    DC_MOTOR,
    LED,
} ev3dev_lego_port_t;

static const char* const port_modes[] = {
    "no-motor",
    "no-sensor",
    "auto",
    "nxt-analog",
    "nxt-color",
    "nxt-i2c",
    "other-i2c",
    "ev3-analog",
    "ev3-uart",
    "other-uart",
    "raw",
    "tacho-motor",
    "dc-motor",
    "led",
};

// Get the port mode
static pbio_error_t ev3dev_lego_port_get_mode(pbio_port_t port, const char *attribute, ev3dev_lego_port_t *port_mode) {
    // Read lego-port number
    int n_lport;
    pbio_error_t err;
    err = sysfs_get_number(port, "/sys/class/lego-port", &n_lport);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get mode file path
    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH, "/sys/class/lego-port/port%d/%s", n_lport, attribute);

    // Open mode file for reading
    char mode[12];
    FILE *f_mode = fopen(path, "r");
    if (f_mode == NULL) {
        return PBIO_ERROR_IO;
    }
    // Read the current mode
    if (fscanf(f_mode, "%" MAX_READ_LENGTH "s", mode) < 1) {
        return PBIO_ERROR_IO;
    }
    // Close the mode file
    if (fclose(f_mode) != 0) {
        return PBIO_ERROR_IO;
    }

    // Find matching port mode string
    for (int i = 0; i < PBIO_ARRAY_SIZE(port_modes); i++) {
        if (!strcmp(mode, port_modes[i])) {
            *port_mode = i;
            return PBIO_SUCCESS;
        }
    }
    return PBIO_ERROR_IO;
}

// Write the port mode without questions
static pbio_error_t ev3dev_lego_port_set_mode(pbio_port_t port, ev3dev_lego_port_t mode) {

    // Read lego-port number
    int n_lport;
    pbio_error_t err;
    err = sysfs_get_number(port, "/sys/class/lego-port", &n_lport);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Mode file path
    char p_mode[MAX_PATH_LENGTH];
    snprintf(p_mode, MAX_PATH_LENGTH, "/sys/class/lego-port/port%d/mode", n_lport);
    // Open mode file for writing
    FILE *f_port_mode = fopen(p_mode, "w");
    if (f_port_mode == NULL) {
        return PBIO_ERROR_IO;
    }
    // Write mode
    if (fprintf(f_port_mode, "%s", port_modes[mode]) != strlen(port_modes[mode])) {
        return PBIO_ERROR_IO;
    }
    // Close the mode file
    if (fclose(f_port_mode) != 0) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

// Set compatible port configuration for given device
pbio_error_t ev3dev_lego_port_configure(pbio_port_t port, pbio_iodev_type_id_t id) {
    
    pbio_error_t err;

    // Get the current port mode and status
    ev3dev_lego_port_t mode_now, status_now;
    err = ev3dev_lego_port_get_mode(port, "mode", &mode_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = ev3dev_lego_port_get_mode(port, "status", &status_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If special modes have been set previously and they're still good, we're done.
    if ((id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR && status_now == RAW       ) ||
        (id == PBIO_IODEV_TYPE_ID_NXT_ANALOG       && status_now == NXT_ANALOG) ||
        (id == PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR && status_now == NXT_ANALOG) ||
        (id == PBIO_IODEV_TYPE_ID_CUSTOM_I2C       && status_now == OTHER_I2C ) ||
        (id == PBIO_IODEV_TYPE_ID_CUSTOM_UART      && status_now == OTHER_UART) ||
        (id == PBIO_IODEV_TYPE_ID_LUMP_UART        && status_now == EV3_UART)   ||
        (id == PBIO_IODEV_TYPE_ID_EV3DEV_DC_MOTOR  && status_now == DC_MOTOR)
    ){
        return PBIO_SUCCESS;
    }

    // For undetected analog sensors, port must be set on first use
    if (id == PBIO_IODEV_TYPE_ID_NXT_ANALOG || id == PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR) {
        err = ev3dev_lego_port_set_mode(port, NXT_ANALOG);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For Custom UART Sensors, port must be set on first use
    if (id == PBIO_IODEV_TYPE_ID_CUSTOM_UART) {
        err = ev3dev_lego_port_set_mode(port, OTHER_UART);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For Custom I2C Sensors, port must be set on first use
    if (id == PBIO_IODEV_TYPE_ID_CUSTOM_I2C) {
        err = ev3dev_lego_port_set_mode(port, OTHER_I2C);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For NXT 2.0 Color Sensor, port must be set to raw mode on first use
    if (id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        err = ev3dev_lego_port_set_mode(port, RAW);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For DC Motors Sensor, port must be set to DC Motor on first use
    if (id == PBIO_IODEV_TYPE_ID_EV3DEV_DC_MOTOR) {
        err = ev3dev_lego_port_set_mode(port, DC_MOTOR);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For custom LUMP sensors, port must be set to EV3 UART on first use
    if (id == PBIO_IODEV_TYPE_ID_LUMP_UART) {
        err = ev3dev_lego_port_set_mode(port, EV3_UART);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For all other devices, the port should be in auto mode.
    if (mode_now != AUTO) {
        err = ev3dev_lego_port_set_mode(port, AUTO);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }
    return PBIO_SUCCESS;
}

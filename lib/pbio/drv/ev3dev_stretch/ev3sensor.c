// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/ev3sensor.h>
#include <pbdrv/ev3devsysfs.h>
#include <pbio/ev3device.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#define MAX_PATH_LENGTH 60
#define MAX_READ_LENGTH "60"

typedef enum {
    AUTO,
    NXT_ANALOG,
    NXT_COLOR,
    NXT_I2C,
    OTHER_I2C,
    EV3_ANALOG,
    EV3_UART,
    OTHER_UART,
    RAW,
} pbdrv_ev3dev_port_t;

static const char* const port_modes[] = {
    "auto",
    "nxt-analog",
    "nxt-color",
    "nxt-i2c",
    "other-i2c",
    "ev3-analog",
    "ev3-uart",
    "other-uart",
    "raw"
};

struct _pbdrv_ev3_sensor_t {
    int n_sensor;
    int n_modes;
    FILE *f_mode;
    FILE *f_driver_name;
    FILE *f_bin_data;
    FILE *f_num_values;
    FILE *f_bin_data_format;
    char modes[12][17];
};

// Get the port mode
static pbio_error_t ev3_sensor_get_port_mode(pbio_port_t port, pbdrv_ev3dev_port_t *port_mode) {
    // Read lego-port number
    int n_lport;
    pbio_error_t err;
    err = sysfs_get_number(port, "/sys/class/lego-port", &n_lport);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get mode file path
    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH, "/sys/class/lego-port/port%d/mode", n_lport);

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
    for (int i = 0; i < sizeof(port_modes)/sizeof(port_modes[0]); i++) {
        if (!strcmp(mode, port_modes[i])) {
            *port_mode = i;
            return PBIO_SUCCESS;
        }
    }
    return PBIO_ERROR_IO;
}

// Write the port mode without questions
static pbio_error_t ev3_sensor_write_port_mode(pbio_port_t port, pbdrv_ev3dev_port_t mode) {

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

// Set port configuration for some devices
static pbio_error_t ev3_sensor_configure_port(pbio_port_t port, pbio_iodev_type_id_t id) {

    // Get the current port mode
    pbio_error_t err;
    pbdrv_ev3dev_port_t mode_now;
    err = ev3_sensor_get_port_mode(port, &mode_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If special modes have been set previously and they're still good, we're done.
    if ((id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR && mode_now == RAW       ) ||
        (id == PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG    && mode_now == NXT_ANALOG) ||
        (id == PBIO_IODEV_TYPE_ID_CUSTOM_I2C       && mode_now == OTHER_I2C ) ||
        (id == PBIO_IODEV_TYPE_ID_CUSTOM_UART      && mode_now == OTHER_UART) ){
        return PBIO_SUCCESS;
    }

    // For Custom Analog Sensors, port must be set on first use
    if (id == PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG) {
        err = ev3_sensor_write_port_mode(port, NXT_ANALOG);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For Custom UART Sensors, port must be set on first use
    if (id == PBIO_IODEV_TYPE_ID_CUSTOM_UART) {
        err = ev3_sensor_write_port_mode(port, OTHER_UART);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For Custom I2C Sensors, port must be set on first use
    if (id == PBIO_IODEV_TYPE_ID_CUSTOM_I2C) {
        err = ev3_sensor_write_port_mode(port, OTHER_I2C);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For NXT 2.0 Color Sensor, port must be set to raw mode on first use
    if (id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        err = ev3_sensor_write_port_mode(port, RAW);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }

    // For all other devices, the port should be in auto mode.
    if (mode_now != AUTO) {
        err = ev3_sensor_write_port_mode(port, AUTO);
        return err == PBIO_SUCCESS ? PBIO_ERROR_AGAIN : err;
    }
    return PBIO_SUCCESS;
}

// Initialize an ev3dev sensor by opening the relevant sysfs attributes
static pbio_error_t ev3_sensor_init(pbdrv_ev3_sensor_t *sensor, pbio_port_t port) {
    pbio_error_t err;

    err = sysfs_get_number(port, "/sys/class/lego-sensor", &sensor->n_sensor);
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open_sensor_attr(&sensor->f_driver_name, sensor->n_sensor, "driver_name", "r");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open_sensor_attr(&sensor->f_mode, sensor->n_sensor, "mode", "r+");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open_sensor_attr(&sensor->f_bin_data_format, sensor->n_sensor, "bin_data_format", "r");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open_sensor_attr(&sensor->f_num_values, sensor->n_sensor, "num_values", "r");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open_sensor_attr(&sensor->f_bin_data, sensor->n_sensor, "bin_data", "rb");
    if (err != PBIO_SUCCESS) { return err; }

    FILE *f_modes;
    err = sysfs_open_sensor_attr(&f_modes, sensor->n_sensor, "modes", "r");
    if (err != PBIO_SUCCESS) { return err; }

    sensor->n_modes = 0;
    while (fscanf(f_modes, " %16s", sensor->modes[sensor->n_modes++]) == 1);
    if (fclose(f_modes) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

// Get the device ID
static pbio_error_t ev3_sensor_assert_id(pbdrv_ev3_sensor_t *sensor, pbio_iodev_type_id_t valid_id) {
    char driver_name[MAX_PATH_LENGTH];

    pbio_error_t err = sysfs_read_str(sensor->f_driver_name, driver_name);
    if (err != PBIO_SUCCESS) { return err; }

    pbio_iodev_type_id_t id;

    // If we are here, we have already confirmed that a lego-sensor exists.
    // So if the user asserts that this should be a lego-sensor, this passes.
    if (valid_id == PBIO_IODEV_TYPE_ID_EV3DEV_LEGO_SENSOR) {
        return PBIO_SUCCESS;
    }

    if (!strcmp(driver_name, "lego-ev3-ir")) {
        id = PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-color")) {
        id = PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-touch")) {
        id = PBIO_IODEV_TYPE_ID_EV3_TOUCH_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-us")) {
        id = PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-gyro")) {
        id = PBIO_IODEV_TYPE_ID_EV3_GYRO_SENSOR;
    }
    else if (!strcmp(driver_name, "nxt-analog")) {
        id = PBIO_IODEV_TYPE_ID_NXT_ANALOG;
    }
    else if (!strncmp(driver_name, "ev3-analog-", 11)) {
        id = PBIO_IODEV_TYPE_ID_EV3_ANALOG;
    }
    else if (!strcmp(driver_name, "lego-nxt-us")) {
        id = PBIO_IODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-nxt-touch")) {
        id = PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-nxt-light")) {
        id = PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR;
    }
    else {
        return PBIO_ERROR_IO;
    }
    // If the detected ID matches the expected ID, return success.
    if (id == valid_id) {
        return PBIO_SUCCESS;
    }
    // Custom NXT Analog sensors work the same as regular NXT Analog sensors.
    if (id == PBIO_IODEV_TYPE_ID_NXT_ANALOG && valid_id == PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_NO_DEV;
}

struct _pbdrv_ev3_sensor_t sensors[4];

// Get an ev3dev sensor
pbio_error_t pbdrv_ev3_sensor_get(pbdrv_ev3_sensor_t **sensor, pbio_port_t port, pbio_iodev_type_id_t valid_id) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *sensor = &sensors[port - PBIO_PORT_1];

    pbio_error_t err;

    // Initialize port if needed for this ID
    err = ev3_sensor_configure_port(port, valid_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // For some custom sensors, there is no
    // lego-sensor to initialize, so we're done.
    if (valid_id == PBIO_IODEV_TYPE_ID_CUSTOM_I2C  ||
        valid_id == PBIO_IODEV_TYPE_ID_CUSTOM_UART ||
        valid_id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        return PBIO_SUCCESS;
    }

    // Initialize sysfs
    err = ev3_sensor_init(*sensor, port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Assert that the expected device is attached
    err = ev3_sensor_assert_id(*sensor, valid_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_SUCCESS;
}

// Get the device info
pbio_error_t pbdrv_ev3_sensor_get_info(pbdrv_ev3_sensor_t *sensor, uint8_t *data_len, pbio_iodev_data_type_t *data_type) {

    pbio_error_t err;

    // Read data length attribute
    int data_len_int;
    err = sysfs_read_int(sensor->f_num_values, &data_len_int);
    if (err != PBIO_SUCCESS) { return err; }
    *data_len = data_len_int;

    // Read data type attribute
    char s_data_type[10];
    err = sysfs_read_str(sensor->f_bin_data_format, s_data_type);
    if (err != PBIO_SUCCESS) { return err; }

    // Convert data type identifier
    if (!strcmp(s_data_type, "s8")) {
        *data_type = PBIO_IODEV_DATA_TYPE_INT8;
        return PBIO_SUCCESS;
    }
    else if (!strcmp(s_data_type, "u8")) {
        *data_type = PBIO_IODEV_DATA_TYPE_UINT8;
        return PBIO_SUCCESS;
    }
    else if (!strcmp(s_data_type, "s16")) {
        *data_type = PBIO_IODEV_DATA_TYPE_INT16;
        return PBIO_SUCCESS;
    }
    else if (!strcmp(s_data_type, "s32")) {
        *data_type = PBIO_IODEV_DATA_TYPE_INT32;
        return PBIO_SUCCESS;
    }
    else if (!strcmp(s_data_type, "float")) {
        *data_type = PBIO_IODEV_DATA_TYPE_FLOAT;
        return PBIO_SUCCESS;
    }

    return PBIO_ERROR_IO;
}

// Set the sensor mode
pbio_error_t pbdrv_ev3_sensor_set_mode(pbdrv_ev3_sensor_t *sensor, uint8_t mode) {

    if (mode > sensor->n_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return sysfs_write_str(sensor->f_mode, sensor->modes[mode]);
}

// Read 32 bytes from bin_data attribute
pbio_error_t pbdrv_ev3_sensor_get_bin_data(pbdrv_ev3_sensor_t *sensor, char *bin_data) {
    if (fseek(sensor->f_bin_data, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fread(bin_data, 1, PBIO_IODEV_MAX_DATA_SIZE, sensor->f_bin_data) < PBIO_IODEV_MAX_DATA_SIZE) {
        return PBIO_ERROR_IO;
    }
    if (fflush(sensor->f_bin_data) != 0) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

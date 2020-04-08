// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <ev3dev_stretch/lego_port.h>
#include <ev3dev_stretch/lego_sensor.h>
#include <ev3dev_stretch/sysfs.h>

#include <pbio/iodev.h>
#include <pbio/port.h>
#include <pbio/util.h>

#define MAX_PATH_LENGTH 60
#define MAX_READ_LENGTH "60"
#define BIN_DATA_SIZE   32 // size of bin_data sysfs attribute

struct _lego_sensor_t {
    int n_sensor;
    int n_modes;
    FILE *f_mode;
    FILE *f_driver_name;
    FILE *f_bin_data;
    FILE *f_num_values;
    FILE *f_bin_data_format;
    char modes[12][17];
    uint8_t bin_data[PBIO_IODEV_MAX_DATA_SIZE]  __attribute__((aligned(32)));
};
// Initialize an ev3dev sensor by opening the relevant sysfs attributes
static pbio_error_t ev3_sensor_init(lego_sensor_t *sensor, pbio_port_t port) {
    pbio_error_t err;

    err = sysfs_get_number(port, "/sys/class/lego-sensor", &sensor->n_sensor);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = sysfs_open_sensor_attr(&sensor->f_driver_name, sensor->n_sensor, "driver_name", "r");
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = sysfs_open_sensor_attr(&sensor->f_mode, sensor->n_sensor, "mode", "r+");
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = sysfs_open_sensor_attr(&sensor->f_bin_data_format, sensor->n_sensor, "bin_data_format", "r");
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = sysfs_open_sensor_attr(&sensor->f_num_values, sensor->n_sensor, "num_values", "r");
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = sysfs_open_sensor_attr(&sensor->f_bin_data, sensor->n_sensor, "bin_data", "rb");
    if (err != PBIO_SUCCESS) {
        return err;
    }

    FILE *f_modes;
    err = sysfs_open_sensor_attr(&f_modes, sensor->n_sensor, "modes", "r");
    if (err != PBIO_SUCCESS) {
        return err;
    }

    sensor->n_modes = 0;
    while (fscanf(f_modes, " %16s", sensor->modes[sensor->n_modes]) == 1) {
        sensor->n_modes++;
    };
    if (fclose(f_modes) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

// Get the device ID
static pbio_error_t ev3_sensor_get_id(lego_sensor_t *sensor, pbio_iodev_type_id_t *id) {
    char driver_name[MAX_PATH_LENGTH];

    pbio_error_t err = sysfs_read_str(sensor->f_driver_name, driver_name);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (!strcmp(driver_name, "lego-ev3-ir")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-color")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-touch")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_TOUCH_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-us")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-gyro")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_GYRO_SENSOR;
    }
    else if (!strcmp(driver_name, "nxt-analog")) {
        *id = PBIO_IODEV_TYPE_ID_NXT_ANALOG;
    }
    else if (!strcmp(driver_name, "lego-nxt-us")) {
        *id = PBIO_IODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-nxt-touch")) {
        *id = PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-nxt-light")) {
        *id = PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-nxt-temp")) {
        *id = PBIO_IODEV_TYPE_ID_NXT_TEMPERATURE_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-power-storage")) {
        *id = PBIO_IODEV_TYPE_ID_NXT_ENERGY_METER;
    }
    else {
        *id = PBIO_IODEV_TYPE_ID_NONE;
    }
    return PBIO_SUCCESS;
}

// Get the device ID
static pbio_error_t ev3_sensor_assert_id(lego_sensor_t *sensor, pbio_iodev_type_id_t valid_id) {

    pbio_error_t err;
    pbio_iodev_type_id_t id;
    err = ev3_sensor_get_id(sensor, &id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If we are here, we have already confirmed that a lego-sensor exists.
    // So if the user asserts that this should be a LUMP or lego-sensor, this passes.
    if (valid_id == PBIO_IODEV_TYPE_ID_LUMP_UART ||
        valid_id == PBIO_IODEV_TYPE_ID_EV3DEV_LEGO_SENSOR) {
        return PBIO_SUCCESS;
    }

    // If the detected ID matches the expected ID, return success.
    if (id == valid_id) {
        return PBIO_SUCCESS;
    }

    // NXT Touch Sensors also pass as NXT Analog
    if (valid_id == PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR && id == PBIO_IODEV_TYPE_ID_NXT_ANALOG) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_NO_DEV;
}

struct _lego_sensor_t sensors[4];

// Get an ev3dev sensor
pbio_error_t lego_sensor_get(lego_sensor_t **sensor, pbio_port_t port, pbio_iodev_type_id_t valid_id) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *sensor = &sensors[port - PBIO_PORT_1];

    pbio_error_t err;

    // Initialize port if needed for this ID
    err = ev3dev_lego_port_configure(port, valid_id);
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
pbio_error_t lego_sensor_get_info(lego_sensor_t *sensor, uint8_t *data_len, lego_sensor_data_type_t *data_type) {

    pbio_error_t err;

    // Read data length attribute
    int data_len_int;
    err = sysfs_read_int(sensor->f_num_values, &data_len_int);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *data_len = data_len_int;

    // Read data type attribute
    char s_data_type[MAX_PATH_LENGTH];
    err = sysfs_read_str(sensor->f_bin_data_format, s_data_type);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Convert data type identifier
    if (!strcmp(s_data_type, "s8")) {
        *data_type = LEGO_SENSOR_DATA_TYPE_INT8;
    }
    else if (!strcmp(s_data_type, "u8")) {
        *data_type = LEGO_SENSOR_DATA_TYPE_UINT8;
    }
    else if (!strcmp(s_data_type, "s16")) {
        *data_type = LEGO_SENSOR_DATA_TYPE_INT16;
    }
    else if (!strcmp(s_data_type, "s32")) {
        *data_type = LEGO_SENSOR_DATA_TYPE_INT32;
    }
    else if (!strcmp(s_data_type, "s16_be")) {
        *data_type = LEGO_SENSOR_DATA_TYPE_INT16_BE;
    }
    else if (!strcmp(s_data_type, "float")) {
        *data_type = LEGO_SENSOR_DATA_TYPE_FLOAT;
    }
    else {
        return PBIO_ERROR_FAILED;
    }

    return PBIO_SUCCESS;
}

// Get the mode id from string
pbio_error_t lego_sensor_get_mode_id_from_str(lego_sensor_t *sensor, const char *mode_str, uint8_t *mode) {

    // Find matching port mode string
    for (int i = 0; i < PBIO_ARRAY_SIZE(sensor->modes); i++) {
        if (!strcmp(mode_str, sensor->modes[i])) {
            *mode = i;
            return PBIO_SUCCESS;
        }
    }

    // Mode not found
    return PBIO_ERROR_INVALID_ARG;
}

// Get the current sensor mode
pbio_error_t lego_sensor_get_mode(lego_sensor_t *sensor, uint8_t *mode) {

    pbio_error_t err;

    // Read mode string
    char mode_str[PBIO_ARRAY_SIZE(sensor->modes[0])];
    err = sysfs_read_str(sensor->f_mode, mode_str);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return matching mode id
    return lego_sensor_get_mode_id_from_str(sensor, mode_str, mode);
}

// Set the sensor mode
pbio_error_t lego_sensor_set_mode(lego_sensor_t *sensor, uint8_t mode) {

    if (mode >= sensor->n_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return sysfs_write_str(sensor->f_mode, sensor->modes[mode]);
}

// Read 32 bytes from bin_data attribute
pbio_error_t lego_sensor_get_bin_data(lego_sensor_t *sensor, uint8_t **bin_data) {
    if (fseek(sensor->f_bin_data, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fread(sensor->bin_data, 1, BIN_DATA_SIZE, sensor->f_bin_data) < BIN_DATA_SIZE) {
        return PBIO_ERROR_IO;
    }

    *bin_data = sensor->bin_data;

    return PBIO_SUCCESS;
}

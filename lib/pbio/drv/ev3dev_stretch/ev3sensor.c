// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/ev3sensor.h>
#include <pbio/ev3device.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#define MAX_PATH_LENGTH 60
#define MAX_READ_LENGTH "60"

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

// Get the ev3dev sensor number for a given port
pbio_error_t sysfs_get_number(pbio_port_t port, int *sysfs_number) {
    // Open lego-sensor directory in sysfs
    DIR *d_sensor;
    struct dirent *entry;
    d_sensor = opendir("/sys/class/lego-sensor");
    if (!d_sensor) {
        return PBIO_ERROR_NO_DEV;
    }
    // Find sensor number for given port
    while ((entry = readdir(d_sensor))) {
        // Ignore the . and .. folders
        if (entry->d_name[0] != '.') {
            // Open the address file to get the port number
            char p_address[MAX_PATH_LENGTH];
            snprintf(p_address, MAX_PATH_LENGTH, "/sys/class/lego-sensor/%s/address", entry->d_name);
            FILE *f_address = fopen(p_address, "r");
            if (f_address == NULL) {
                return PBIO_ERROR_IO;
            }

            // Get the port from the address file
            fseek(f_address, 12, SEEK_SET);
            pbio_port_t port_found = fgetc(f_address);
            fclose(f_address);

            // If the port matches the requested port, get where it was found.
            if (port_found == port) {
                sscanf(entry->d_name, "%*6c%d",  sysfs_number);
                closedir(d_sensor);
                return PBIO_SUCCESS;
            }
        }
    }
    // No sensor was found at the requested port
    closedir(d_sensor);
    return PBIO_ERROR_NO_DEV;
}

// Open a sysfs attribute
pbio_error_t sysfs_open(FILE **file, int n, const char *attribute, const char *rw) {
    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH, "/sys/class/lego-sensor/sensor%d/%s", n, attribute);
    *file = fopen(path, rw);
    return file == NULL? PBIO_ERROR_IO : PBIO_SUCCESS;
}

// Read a string from a previously opened sysfs attribute
pbio_error_t sysfs_read_str(FILE *file, char *dest) {
    if (fseek(file, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(file, "%" MAX_READ_LENGTH "s", dest) < 1) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

// Read an int from a previously opened sysfs attribute
pbio_error_t sysfs_read_int(FILE *file, int *dest) {
    if (fseek(file, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(file, "%d", dest) < 1) {
        return PBIO_ERROR_IO;
    }

    if (fflush(file) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

// Initialize an ev3dev sensor by opening the relevant sysfs attributes
pbio_error_t ev3_sensor_init(pbdrv_ev3_sensor_t *sensor, pbio_port_t port) {
    pbio_error_t err;

    err = sysfs_get_number(port, &sensor->n_sensor) ;
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&sensor->f_driver_name, sensor->n_sensor, "driver_name", "r");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&sensor->f_mode, sensor->n_sensor, "mode", "r+");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&sensor->f_bin_data_format, sensor->n_sensor, "bin_data_format", "r");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&sensor->f_num_values, sensor->n_sensor, "num_values", "r");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&sensor->f_bin_data, sensor->n_sensor, "bin_data", "rb");
    if (err != PBIO_SUCCESS) { return err; }

    FILE *f_modes;
    err = sysfs_open(&f_modes, sensor->n_sensor, "modes", "r");
    if (err != PBIO_SUCCESS) { return err; }
    
    sensor->n_modes = 0;
    while (fscanf(f_modes, " %16s", sensor->modes[sensor->n_modes++]) == 1);
    if (fclose(f_modes) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

struct _pbdrv_ev3_sensor_t sensors[4];

// Get an ev3dev sensor
pbio_error_t pbdrv_ev3_sensor_get(pbdrv_ev3_sensor_t **sensor, pbio_port_t port) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *sensor = &sensors[port - PBIO_PORT_1];

    return ev3_sensor_init(*sensor, port);
}

// Get the device ID
pbio_error_t pbdrv_ev3_sensor_get_id(pbdrv_ev3_sensor_t *sensor, pbio_iodev_type_id_t *id) {
    char driver_name[MAX_PATH_LENGTH];

    pbio_error_t err = sysfs_read_str(sensor->f_driver_name, driver_name);
    if (err != PBIO_SUCCESS) { return err; }

    if (!strcmp(driver_name, "lego-ev3-ir")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-color")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR;
    }
    else if (!strcmp(driver_name, "lego-ev3-touch")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_TOUCH_SENSOR;
    }
    else {
        return PBIO_ERROR_IO;
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

    // sysfs identifier for mode
    char *sysfs_mode = sensor->modes[mode];

    // Write mode identifier
    if (fseek(sensor->f_mode, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fprintf(sensor->f_mode, sysfs_mode) != strlen(sysfs_mode)) {
        return PBIO_ERROR_IO;
    }
    
    if (fflush(sensor->f_mode) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
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

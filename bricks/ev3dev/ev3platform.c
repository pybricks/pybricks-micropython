// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#include "ev3platform.h"

#define MAX_PATH_LENGTH 60
#define MAX_READ_LENGTH "60"

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

// Initialize an ev3dev sensor by opening the relevant sysfs attributes
pbio_error_t ev3_sensor_init(ev3_platform_t *platform, pbio_port_t port) {
    pbio_error_t err;

    err = sysfs_get_number(port, &platform->n_sensor) ;
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&platform->f_driver_name, platform->n_sensor, "driver_name", "r");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&platform->f_mode, platform->n_sensor, "mode", "w");
    if (err != PBIO_SUCCESS) { return err; }

    err = sysfs_open(&platform->f_bin_data, platform->n_sensor, "bin_data", "rb");

    return PBIO_SUCCESS;
}

// Get the device ID
pbio_error_t ev3_sensor_get_id(ev3_platform_t *platform, pbio_iodev_type_id_t *id) {
    char driver_name[MAX_PATH_LENGTH];

    pbio_error_t err = sysfs_read_str(platform->f_driver_name, driver_name);
    if (err != PBIO_SUCCESS) { return err; }

    if (!strcmp(driver_name, "lego-ev3-ir")) {
        *id = PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR;
        return PBIO_SUCCESS;
    }

    return PBIO_ERROR_IO;
}

// Read 32 bytes from bin_data attribute
pbio_error_t ev3_sensor_get_bin_data(ev3_platform_t *platform, char *bin_data) {
    if (fseek(platform->f_bin_data, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fread(bin_data, 1, PBIO_IODEV_MAX_DATA_SIZE, platform->f_bin_data) < PBIO_IODEV_MAX_DATA_SIZE) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

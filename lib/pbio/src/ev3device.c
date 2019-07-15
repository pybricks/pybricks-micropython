// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#include <pbio/ev3device.h>

ev3_iodev_t iodevices[4];

pbio_error_t ev3device_get_device(ev3_iodev_t **iodev, pbio_port_t port) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *iodev = &iodevices[port - PBIO_PORT_1];
    ev3_platform_t *platform = &(*iodev)->platform;

    pbio_error_t err;

    err = ev3_sensor_init(platform, port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set the device port
    (*iodev)->port = port;

    // Get device ID
    err = ev3_sensor_get_id(platform, &(*iodev)->type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // discuss connect disconnect, should break even if same type connected...
    return PBIO_SUCCESS;
}

pbio_error_t ev3device_get_values_at_mode(ev3_iodev_t *iodev, pbio_iodev_type_id_t valid_id, uint8_t mode, void *values) {

    // Assert device is of the expected type
    if (iodev->type_id != valid_id) {
        return PBIO_ERROR_NO_DEV;
    }

    // Set the mode if not already set

    // Read raw data from device
    char bin_data[PBIO_IODEV_MAX_DATA_SIZE];
    ev3_sensor_get_bin_data(&iodev->platform, bin_data);

    // Convert data to correct type for this device and mode
    for (int i = 0; i < PBIO_IODEV_MAX_DATA_SIZE; i++) {
        printf("%d: %d\n", i, (int) bin_data[i]);
    }

    return PBIO_SUCCESS;
}

// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#include <pbio/ev3device.h>

pbio_ev3iodev_t iodevices[4];

// Get an ev3dev sensor
pbio_error_t ev3device_get_device(pbio_ev3iodev_t **iodev, pbio_port_t port) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbio_ev3iodev_t *_iodev = &iodevices[port - PBIO_PORT_1];

    *iodev = _iodev;

    _iodev->port = port;

    pbio_error_t err;

    err = pbdrv_ev3_sensor_get(&_iodev->sensor, _iodev->port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get device ID
    err = pbdrv_ev3_sensor_get_id(_iodev->sensor, &_iodev->type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}


pbio_error_t ev3device_get_values_at_mode(pbio_ev3iodev_t *iodev, pbio_iodev_mode_id_t mode, void *values) {

    pbio_error_t err;

    // Assert that device is of the expected type
    if (iodev->type_id != mode >> 8) {
        return PBIO_ERROR_NO_DEV;
    }

    // Set the mode only if not already set, or if this sensor mode requires it
    if (iodev->mode != mode || mode == 0) {
        err = pbdrv_ev3_sensor_set_mode(iodev->sensor, mode);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Set the new mode and corresponding data info
        iodev->mode = mode;
        err = pbdrv_ev3_sensor_get_info(iodev->sensor, &iodev->data_len, &iodev->data_type);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }    

    // Read raw data from device
    char data[PBIO_IODEV_MAX_DATA_SIZE];
    pbdrv_ev3_sensor_get_bin_data(iodev->sensor, data);

    for (uint8_t i = 0; i < iodev->data_len; i++) {
        switch (iodev->data_type) {
        case PBIO_IODEV_DATA_TYPE_INT8:
            memcpy((int8_t  *) values + i * 1, (int8_t  *)(data + i * 1), 1);
            break;
        case PBIO_IODEV_DATA_TYPE_INT16:
            memcpy((int8_t  *) values + i * 2, (int16_t *)(data + i * 2), 2);
            break;
        case PBIO_IODEV_DATA_TYPE_INT32:
            memcpy((int8_t  *) values + i * 4, (int32_t *)(data + i * 4), 4);
            break;
        case PBIO_IODEV_DATA_TYPE_FLOAT:
            memcpy((int8_t  *) values + i * 4, (float   *)(data + i * 4), 4);
            break;
        default:
            return PBIO_ERROR_IO;
        }
    }

    return PBIO_SUCCESS;
}

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
pbio_error_t ev3device_get_device(pbio_ev3iodev_t **iodev, pbio_iodev_type_id_t valid_id, pbio_port_t port) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbio_ev3iodev_t *_iodev = &iodevices[port - PBIO_PORT_1];

    *iodev = _iodev;

    _iodev->port = port;
    _iodev->mode = 255;

    pbio_error_t err;

    err = pbdrv_ev3_sensor_get(&_iodev->sensor, _iodev->port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get and verify device ID
    err = pbdrv_ev3_sensor_get_id(_iodev->sensor, &_iodev->type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    if (_iodev->type_id != valid_id) {
        return PBIO_ERROR_NO_DEV;
    }

    return PBIO_SUCCESS;
}


pbio_error_t ev3device_get_values_at_mode(pbio_ev3iodev_t *iodev, uint8_t mode, void *values) {

    pbio_error_t err;
    // Set the mode if not already set
    if (iodev->mode != mode || (
        // and also if this sensor/mode requires setting it every time:
        iodev->type_id == PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR && mode >= PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM
    )) {
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

    err = pbdrv_ev3_sensor_get_bin_data(iodev->sensor, data);
    if (err != PBIO_SUCCESS) {
        return err;
    }

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

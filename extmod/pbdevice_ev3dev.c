// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk


#include "pbdevice.h"

#include <pbio/config.h>

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#include <ev3dev_stretch/nxtcolor.h>

pbdevice_t iodevices[4];

// Get an ev3dev sensor
pbio_error_t pbdevice_get_device(pbdevice_t **pbdev, pbio_iodev_type_id_t valid_id, pbio_port_t port) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbdevice_t *_pbdev = &iodevices[port - PBIO_PORT_1];

    *pbdev = _pbdev;

    _pbdev->port = port;
    _pbdev->mode = 255;

    pbio_error_t err;

    // Get the device and assert that is has a valid id
    err = lego_sensor_get(&_pbdev->sensor, _pbdev->port, valid_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    _pbdev->type_id = valid_id;

    return PBIO_SUCCESS;
}


pbio_error_t pbdevice_get_values(pbdevice_t *pbdev, uint8_t mode, void *values) {

    // The NXT Color Sensor is a special case, so deal with it accordingly
    if (pbdev->type_id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        return nxtcolor_get_values_at_mode(pbdev->port, mode, values);
    }

    pbio_error_t err;
    // Set the mode if not already set
    if (pbdev->mode != mode || (
        // and also if this sensor/mode requires setting it every time:
        pbdev->type_id == PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR && mode >= PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM
    )) {
        err = lego_sensor_set_mode(pbdev->sensor, mode);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Set the new mode and corresponding data info
        pbdev->mode = mode;
        err = lego_sensor_get_info(pbdev->sensor, &pbdev->data_len, &pbdev->data_type);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Read raw data from device
    char data[PBIO_IODEV_MAX_DATA_SIZE];

    err = lego_sensor_get_bin_data(pbdev->sensor, data);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    for (uint8_t i = 0; i < pbdev->data_len; i++) {
        switch (pbdev->data_type) {
            case PBIO_IODEV_DATA_TYPE_UINT8:
            memcpy((int8_t  *) values + i * 1, (uint8_t *)(data + i * 1), 1);
            break;
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

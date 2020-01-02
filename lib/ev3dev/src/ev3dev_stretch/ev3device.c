// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

// Object for interacting with LEGO MINDSTORMS EV3 input devices

#include <pbio/config.h>

#if PBIO_CONFIG_EV3_INPUT_DEVICE

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#include <ev3device.h>
#include <ev3dev_stretch/nxtcolor.h>

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

    // Get the device and assert that is has a valid id
    err = lego_sensor_get(&_iodev->sensor, _iodev->port, valid_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    _iodev->type_id = valid_id;

    return PBIO_SUCCESS;
}


pbio_error_t ev3device_get_values_at_mode(pbio_ev3iodev_t *iodev, uint8_t mode, void *values) {

    // The NXT Color Sensor is a special case, so deal with it accordingly
    if (iodev->type_id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        return nxtcolor_get_values_at_mode(iodev->port, mode, values);
    }

    pbio_error_t err;
    // Set the mode if not already set
    if (iodev->mode != mode || (
        // and also if this sensor/mode requires setting it every time:
        iodev->type_id == PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR && mode >= PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM
    )) {
        err = lego_sensor_set_mode(iodev->sensor, mode);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Set the new mode and corresponding data info
        iodev->mode = mode;
        err = lego_sensor_get_info(iodev->sensor, &iodev->data_len, &iodev->data_type);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Read raw data from device
    char data[PBIO_IODEV_MAX_DATA_SIZE];

    err = lego_sensor_get_bin_data(iodev->sensor, data);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    for (uint8_t i = 0; i < iodev->data_len; i++) {
        switch (iodev->data_type) {
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

#endif // PBIO_CONFIG_EV3_INPUT_DEVICE

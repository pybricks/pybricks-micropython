// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include "py/mphal.h"

#include "pberror.h"
#include "pbdevice.h"

#include <pbio/config.h>

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#include <ev3dev_stretch/lego_sensor.h>
#include <ev3dev_stretch/nxtcolor.h>

struct _pbdevice_t {
    /**
     * The device ID
     */
    pbio_iodev_type_id_t type_id;
    /**
     * The port the device is attached to.
     */
    pbio_port_t port;
    /**
     * The current active mode.
     */
    uint8_t mode;
    /**
     * The number of values for current mode.
     */
    uint8_t data_len;
    /**
     * Data type for current mode
     */
    lego_sensor_data_type_t data_type;
    /**
     * Platform specific low-level device abstraction
     */
    lego_sensor_t *sensor;
};

pbdevice_t iodevices[4];

// Get an ev3dev sensor
static pbio_error_t get_device(pbdevice_t **pbdev, pbio_iodev_type_id_t valid_id, pbio_port_t port) {
    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbdevice_t *_pbdev = &iodevices[port - PBIO_PORT_1];

    _pbdev->port = port;

    pbio_error_t err;

    // Get the device and assert that is has a valid id
    err = lego_sensor_get(&_pbdev->sensor, _pbdev->port, valid_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    _pbdev->type_id = valid_id;

    // For special sensor classes we are done. No need to read mode.
    if (valid_id == PBIO_IODEV_TYPE_ID_CUSTOM_I2C ||
        valid_id == PBIO_IODEV_TYPE_ID_CUSTOM_UART ||
        valid_id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        *pbdev = _pbdev;
        return PBIO_SUCCESS;
    }

    // Get mode
    err = lego_sensor_get_mode(_pbdev->sensor, &_pbdev->mode);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Get corresponding data info
    err = lego_sensor_get_info(_pbdev->sensor, &_pbdev->data_len, &_pbdev->data_type);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return pointer to device on success
    *pbdev = _pbdev;

    return PBIO_SUCCESS;
}

// Get the required mode switch time delay for a given sensor type and/or mode
static uint32_t get_mode_switch_delay(pbio_iodev_type_id_t id, uint8_t mode) {
    switch (id) {
        case PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR:
            return 30;
        case PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR:
            return 1100;
        case PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR:
            return 20;
        case PBIO_IODEV_TYPE_ID_NXT_SOUND_SENSOR:
            return 300;
        case PBIO_IODEV_TYPE_ID_NXT_ENERGY_METER:
            return 200;
        // Default delay for other sensors and modes:
        default:
            return 0;
    }
}

static pbio_error_t get_values(pbdevice_t *pbdev, uint8_t mode, int32_t *values) {

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

        // Give some time for the mode to take effect and discard stale data
        uint32_t delay = get_mode_switch_delay(pbdev->type_id, mode);
        if (delay > 0) {
            mp_hal_delay_ms(delay);
        }
    }

    // Read raw data from device
    uint8_t *data;

    err = lego_sensor_get_bin_data(pbdev->sensor, &data);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    for (uint8_t i = 0; i < pbdev->data_len; i++) {
        switch (pbdev->data_type) {
            case LEGO_SENSOR_DATA_TYPE_UINT8:
                values[i] = *((uint8_t *)(data + i * 1));
                break;
            case LEGO_SENSOR_DATA_TYPE_INT8:
                values[i] = *((int8_t *)(data + i * 1));
                break;
            case LEGO_SENSOR_DATA_TYPE_INT16:
                values[i] = *((int16_t *)(data + i * 2));
                break;
            case LEGO_SENSOR_DATA_TYPE_UINT16:
                values[i] = *((uint16_t *)(data + i * 2));
                break;
            case LEGO_SENSOR_DATA_TYPE_INT32:
                values[i] = *((int32_t *)(data + i * 4));
                break;
            case LEGO_SENSOR_DATA_TYPE_UINT32:
                values[i] = *((uint32_t *)(data + i * 4));
                break;
            case LEGO_SENSOR_DATA_TYPE_INT16_BE:
                values[i] = (int16_t)__builtin_bswap16(*(uint16_t *)(data + i * 2));
                break;
            case LEGO_SENSOR_DATA_TYPE_FLOAT:
                *(float *)(values + i) = *((float *)(data + i * 4));
                break;
            default:
                return PBIO_ERROR_IO;
        }
    }

    return PBIO_SUCCESS;
}

pbdevice_t *pbdevice_get_device(pbio_port_t port, pbio_iodev_type_id_t valid_id) {
    pbdevice_t *pbdev = NULL;
    pbio_error_t err;

    // Try to get the device
    err = get_device(&pbdev, valid_id, port);

    // FIXME: Reading port mode is not enough confirmation that we are done,
    // So we cannot wait until PBIO_ERROR_AGAIN disappears. Use udev instead.
    // For now, just wait a little longer before giving up.
    if (err == PBIO_ERROR_AGAIN) {
        for (uint8_t i = 0; i < 5; i++) {
            err = get_device(&pbdev, valid_id, port);
            if (err == PBIO_SUCCESS) {
                break;
            }
            mp_hal_delay_ms(1000 * (i + 1));
        }
    }
    pb_assert(err);
    return pbdev;
}
void pbdevice_get_values(pbdevice_t *pbdev, uint8_t mode, int32_t *values) {
    pbio_error_t err;
    while ((err = get_values(pbdev, mode, values)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1);
    }
    pb_assert(err);
}

void pbdevice_set_values(pbdevice_t *pbdev, uint8_t mode, int32_t *values, uint8_t num_values) {
    pb_assert(PBIO_ERROR_NOT_SUPPORTED);
}

void pbdevice_set_power_supply(pbdevice_t *pbdev, int32_t duty) {
    pb_assert(PBIO_ERROR_NOT_SUPPORTED);
}

void pbdevice_get_info(pbdevice_t *pbdev, pbio_port_t *port, pbio_iodev_type_id_t *id, uint8_t *mode, uint8_t *num_values) {
    *port = pbdev->port;
    *id = pbdev->type_id;
    *mode = pbdev->mode;
    *num_values = pbdev->data_len;
}

int8_t pbdevice_get_mode_id_from_str(pbdevice_t *pbdev, const char *mode_str) {
    uint8_t mode;
    pb_assert(lego_sensor_get_mode_id_from_str(pbdev->sensor, mode_str, &mode));
    return mode;
}

void pbdevice_color_light_on(pbdevice_t *pbdev, pbio_light_color_t color) {
    // Turn on the light through device specific mode
    uint8_t mode;
    switch (pbdev->type_id) {
        case PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR:
            switch (color) {
                case PBIO_LIGHT_COLOR_GREEN:
                    mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_G;
                    break;
                case PBIO_LIGHT_COLOR_RED:
                    mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_R;
                    break;
                case PBIO_LIGHT_COLOR_BLUE:
                    mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_B;
                    break;
                default:
                    mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_OFF;
                    break;
            }
            int32_t unused;
            pbdevice_get_values(pbdev, mode, &unused);
            break;
        default:
            pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    }
}

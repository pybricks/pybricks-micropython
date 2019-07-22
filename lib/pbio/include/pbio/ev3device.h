// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_EV3DEVICE_H_
#define _PBIO_EV3DEVICE_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

#include <pbdrv/ev3sensor.h>

// TODO: Make structure like iodevice
typedef struct _pbio_ev3iodev_t {
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
     * The current active mode.
     */
    uint8_t data_len;
    /**
     * The current active mode.
     */
    pbio_iodev_data_type_t data_type;
    /**
     * Platform specific low-level device abstraction
     */
    pbdrv_ev3_sensor_t *sensor;
} pbio_ev3iodev_t;

pbio_error_t ev3device_get_device(pbio_ev3iodev_t **iodev, pbio_iodev_type_id_t valid_id, pbio_port_t port);

pbio_error_t ev3device_get_values_at_mode(pbio_ev3iodev_t *iodev, uint8_t mode, void *values);

/**
 * Mode identifiers for EV3 devices.
 */

// LEGO MINDSTORMS EV3 Touch Sensor
enum {    
    PBIO_IODEV_MODE_ID_EV3_TOUCH_SENSOR__TOUCH   = 0,
};

// LEGO MINDSTORMS EV3 Color Sensor
enum {    
    PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__COL_REFLECT   = 0,
    PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__COL_AMBIENT   = 1,
    PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__COL_COLOR     = 2,
    PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__REF_RAW       = 3,
    PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__RGB_RAW       = 4,
    PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__COL_CAL       = 5,
};

// LEGO MINDSTORMS EV3 Infrared Sensor
enum {
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_PROX          = 0,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_SEEK          = 1,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_REMOTE        = 2,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_REM_A         = 3,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_S_ALT         = 4,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_CAL           = 5,
};

#endif // _PBIO_EV3DEVICE_H_

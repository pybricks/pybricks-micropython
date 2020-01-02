// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef _PBDEVICE_H_
#define _PBDEVICE_H_

#include <stdint.h>


#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include <ev3dev_stretch/lego_sensor.h>

// TODO: Make structure like iodevice
typedef struct _pbdevice_t {
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
    lego_sensor_t *sensor;
} pbdevice_t;

pbio_error_t pbdevice_get_device(pbdevice_t **pbdev, pbio_iodev_type_id_t valid_id, pbio_port_t port);

pbio_error_t pbdevice_get_values(pbdevice_t *pbdev, uint8_t mode, void *values);

/**
 * Mode identifiers for EV3 devices.
 */

// LEGO MINDSTORMS EV3 Touch Sensor
enum {
    PBIO_IODEV_MODE_EV3_TOUCH_SENSOR__TOUCH        = 0,
};

// LEGO MINDSTORMS EV3 Color Sensor
enum {
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__REFLECT      = 0,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__AMBIENT      = 1,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__COLOR        = 2,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__REF_RAW      = 3,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__RGB_RAW      = 4,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__CAL          = 5,
};

// LEGO MINDSTORMS EV3 Infrared Sensor
enum {
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__PROX      = 0,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__SEEK      = 1,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REMOTE    = 2,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REM_A     = 3,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__S_ALT     = 4,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__CAL       = 5,
};

// LEGO MINDSTORMS EV3 Ultrasonic Sensor
enum {
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM = 0,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_IN = 1,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__LISTEN  = 2,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM   = 3,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_IN   = 4,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DC_CM   = 5,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DC_IN   = 6,
};

// LEGO MINDSTORMS EV3 Gyro Sensor
enum {
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__ANG           = 0,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__RATE          = 1,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__FAS           = 2,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__G_A           = 3,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__CAL           = 4,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__RATE2         = 5,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__ANG2          = 6,
};

// LEGO MINDSTORMS EV3 Analog Sensor
enum {
    PBIO_IODEV_MODE_EV3_ANALOG__RAW                = 0,
};

/**
 * Mode identifiers for NXT devices.
 */

// LEGO MINDSTORMS NXT Analog Sensor
enum {
    PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE            = 0,
    PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE             = 1,
};

// LEGO MINDSTORMS NXT Ultrasonic Sensor
enum {
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM = 0,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_IN = 1,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__SI_CM   = 2,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__SI_IN   = 3,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__LISTEN  = 4,
};

// LEGO MINDSTORMS NXT Light Sensor
enum {
    PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__REFLECT      = 0,
    PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__AMBIENT      = 1,
};

// LEGO MINDSTORMS NXT Color Sensor
enum {
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP         = 0,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE      = 1,
};

#endif // _PBDEVICE_H_

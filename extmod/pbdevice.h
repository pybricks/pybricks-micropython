// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBDEVICE_H_
#define _PBDEVICE_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/light.h>

typedef struct _pbdevice_t pbdevice_t;

pbdevice_t *pbdevice_get_device(pbio_port_t port, pbio_iodev_type_id_t valid_id);

void pbdevice_get_values(pbdevice_t *pbdev, uint8_t mode, int32_t *values);

void pbdevice_set_values(pbdevice_t *pbdev, uint8_t mode, int32_t *values, uint8_t num_values);

void pbdevice_set_power_supply(pbdevice_t *pbdev, int32_t duty);

void pbdevice_get_info(pbdevice_t *pbdev, pbio_port_t *port, pbio_iodev_type_id_t *id, uint8_t *mode, uint8_t *num_values);

int8_t pbdevice_get_mode_id_from_str(pbdevice_t *pbdev, const char *mode_str);

void pbdevice_color_light_on(pbdevice_t *pbdev, pbio_light_color_t color);

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
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE      = 0,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_R       = 1,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_G       = 2,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_B       = 3,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_OFF     = 4,
};

// LEGO MINDSTORMS NXT Temperature Sensor
enum {
    PBIO_IODEV_MODE_NXT_TEMPERATURE_SENSOR_CELCIUS = 0,
};

// LEGO MINDSTORMS NXT Energy Meter
enum {
    PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL           = 7,
};

// LEGO POWERED UP WEDO 2.0 Infrared "Motion" sensor
enum {
    PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__DETECT = 0,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT  = 1,  // read 1x int32_t
    PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL    = 2,  // read 3x int16_t
};

// LEGO POWERED UP Color and Distance Sensor
enum {
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__COLOR = 0,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX  = 1,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__COUNT = 2,  // read 1x int32_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__REFLT = 3,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI  = 4,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__COL_O = 5,  // writ 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I = 6,  // read 3x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX = 7,  // writ 1x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1 = 8,  // rrwr 4x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__DEBUG = 9,  // ?? 2x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__CALIB = 10, // ?? 8x int16_t
};

// LEGO SPIKE Color Sensor
enum {
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__COLOR = 0,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__REFLT = 1,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__AMBI  = 2,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__LIGHT = 3,  // writ 3x int8_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__RREFL = 4,  // read 2x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__RGB_I = 5,  // read 4x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__HSV   = 6,  // read 3x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__SHSV  = 7,  // read 4x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__DEBUG = 8,  // ??   2x int16_t
    PBIO_IODEV_MODE_PUP_COLOR_SENSOR__CALIB = 9,  // ??   7x int16_t
};

// LEGO SPIKE Ultrasonic Sensor
enum {
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL = 0,  // read 1x int16_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTS = 1,  // read 1x int16_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__SINGL = 2,  // read 1x int16_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__LISTN = 3,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__TRAW  = 4,  // read 1x int32_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__LIGHT = 5,  // writ 4x int8_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__PING  = 6,  // ??   1x int8_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__ADRAW = 7,  // read 1x int16_t
    PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__CALIB = 8,  // ??   7x int16_t
};

// LEGO SPIKE Force Sensor
enum {
    PBIO_IODEV_MODE_PUP_FORCE_SENSOR__FRAW  = 4,  // read 1x int16_t
    PBIO_IODEV_MODE_PUP_FORCE_SENSOR__CALIB = 6,  // ??   8x int16_t
};

#endif // _PBDEVICE_H_

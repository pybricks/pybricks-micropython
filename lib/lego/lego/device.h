// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2025 The Pybricks Authors

// Known LEGO Device information.

#ifndef _LEGO_DEVICES_H_
#define _LEGO_DEVICES_H_

#include <stdint.h>

#include "lump.h"

/**
 * Type identifiers for LEGO devices.
 *
 * UART device type IDs are hard-coded in the device, so those numbers can't
 * change. Other ID numbers are arbitrary.
 */
typedef enum {
    LEGO_DEVICE_TYPE_ID_NONE                     = 0,    /**< No device is present */

    // LEGO Powered Up non-UART devices - some of these don't exist in real-life
    LEGO_DEVICE_TYPE_ID_LPF2_MMOTOR              = 1,    /**< 45303 Powered Up Medium Motor (aka WeDo 2.0 motor) */
    LEGO_DEVICE_TYPE_ID_LPF2_TRAIN               = 2,    /**< Powered Up Train Motor */
    /** @cond INTERNAL */
    LEGO_DEVICE_TYPE_ID_LPF2_TURN                = 3,
    LEGO_DEVICE_TYPE_ID_LPF2_POWER               = 4,
    LEGO_DEVICE_TYPE_ID_LPF2_TOUCH               = 5,
    LEGO_DEVICE_TYPE_ID_LPF2_LMOTOR              = 6,
    LEGO_DEVICE_TYPE_ID_LPF2_XMOTOR              = 7,
    /** @endcond */
    LEGO_DEVICE_TYPE_ID_LPF2_LIGHT               = 8,    /**< 88005 Powered Up Lights */
    /** @cond INTERNAL */
    LEGO_DEVICE_TYPE_ID_LPF2_LIGHT1              = 9,
    LEGO_DEVICE_TYPE_ID_LPF2_LIGHT2              = 10,
    LEGO_DEVICE_TYPE_ID_LPF2_TPOINT              = 11,
    LEGO_DEVICE_TYPE_ID_LPF2_EXPLOD              = 12,
    LEGO_DEVICE_TYPE_ID_LPF2_3_PART              = 13,
    /** @endcond */
    LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART        = 14,   /**< Temporary ID for UART devices until real ID is read from the device */

    // LEGO EV3 UART devices

    /** MINDSTORMS EV3 Color Sensor */
    LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR = LUMP_TYPE_ID_EV3_COLOR_SENSOR,

    /** MINDSTORMS EV3 Ultrasonic Sensor */
    LEGO_DEVICE_TYPE_ID_EV3_ULTRASONIC_SENSOR = LUMP_TYPE_ID_EV3_ULTRASONIC_SENSOR,

    /** MINDSTORMS EV3 Gyro Sensor */
    LEGO_DEVICE_TYPE_ID_EV3_GYRO_SENSOR = LUMP_TYPE_ID_EV3_GYRO_SENSOR,

    /** MINDSTORMS EV3 Infrared Sensor */
    LEGO_DEVICE_TYPE_ID_EV3_IR_SENSOR = LUMP_TYPE_ID_EV3_IR_SENSOR,


    // WeDo 2.0 UART devices

    /** WeDo 2.0 Tilt Sensor */
    LEGO_DEVICE_TYPE_ID_WEDO2_TILT_SENSOR = LUMP_TYPE_ID_WEDO2_TILT_SENSOR,

    /** WeDo 2.0 Motion Sensor */
    LEGO_DEVICE_TYPE_ID_WEDO2_MOTION_SENSOR = LUMP_TYPE_ID_WEDO2_MOTION_SENSOR,

    LEGO_DEVICE_TYPE_ID_WEDO2_GENERIC_SENSOR = 36,


    // BOOST UART devices and motors

    /** BOOST Color and Distance Sensor */
    LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR = LUMP_TYPE_ID_COLOR_DIST_SENSOR,

    /** BOOST Interactive Motor */
    LEGO_DEVICE_TYPE_ID_INTERACTIVE_MOTOR = LUMP_TYPE_ID_INTERACTIVE_MOTOR,

    /** BOOST Move Hub built-in Motor */
    LEGO_DEVICE_TYPE_ID_MOVE_HUB_MOTOR = 39,

    // Technic motors

    /** Technic Large Motor */
    LEGO_DEVICE_TYPE_ID_TECHNIC_L_MOTOR = LUMP_TYPE_ID_TECHNIC_L_MOTOR,

    /** Technic XL Motor */
    LEGO_DEVICE_TYPE_ID_TECHNIC_XL_MOTOR = LUMP_TYPE_ID_TECHNIC_XL_MOTOR,


    // SPIKE motors

    /** SPIKE Medium Motor */
    LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR = LUMP_TYPE_ID_SPIKE_M_MOTOR,

    /** SPIKE Large Motor */
    LEGO_DEVICE_TYPE_ID_SPIKE_L_MOTOR = LUMP_TYPE_ID_SPIKE_L_MOTOR,

    // SPIKE sensors

    /** SPIKE Color Sensor */
    LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR = LUMP_TYPE_ID_SPIKE_COLOR_SENSOR,

    /** SPIKE Ultrasonic Sensor */
    LEGO_DEVICE_TYPE_ID_SPIKE_ULTRASONIC_SENSOR = LUMP_TYPE_ID_SPIKE_ULTRASONIC_SENSOR,

    /** SPIKE Prime Force Sensor */
    LEGO_DEVICE_TYPE_ID_SPIKE_FORCE_SENSOR = LUMP_TYPE_ID_SPIKE_FORCE_SENSOR,

    /** Technic Color Light Matrix */
    LEGO_DEVICE_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX = LUMP_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX,

    /** SPIKE Small Motor */
    LEGO_DEVICE_TYPE_ID_SPIKE_S_MOTOR = LUMP_TYPE_ID_SPIKE_S_MOTOR,

    // Technic Angular Motors
    LEGO_DEVICE_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR = LUMP_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR,

    LEGO_DEVICE_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR = LUMP_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR,

    // NXT Devices
    LEGO_DEVICE_TYPE_ID_NXT_TOUCH_SENSOR,                /**< MINDSTORMS NXT Touch Sensor */
    LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR,                /**< MINDSTORMS NXT Light Sensor */
    LEGO_DEVICE_TYPE_ID_NXT_SOUND_SENSOR,                /**< MINDSTORMS NXT Sound Sensor */
    LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR,                /**< MINDSTORMS NXT Color Sensor */
    LEGO_DEVICE_TYPE_ID_NXT_ULTRASONIC_SENSOR,           /**< MINDSTORMS NXT Ultrasonic Sensor */
    LEGO_DEVICE_TYPE_ID_NXT_TEMPERATURE_SENSOR,          /**< MINDSTORMS NXT Ultrasonic Sensor */
    LEGO_DEVICE_TYPE_ID_NXT_ENERGY_METER,                /**< MINDSTORMS NXT Energy Meter */
    LEGO_DEVICE_TYPE_ID_NXT_MOTOR,                       /**< MINDSTORMS NXT Motor */

    // EV3 Devices
    LEGO_DEVICE_TYPE_ID_EV3_TOUCH_SENSOR,                /**< MINDSTORMS EV3 Touch Sensor */
    LEGO_DEVICE_TYPE_ID_EV3_LARGE_MOTOR,                 /**< MINDSTORMS EV3 Large Motor */
    LEGO_DEVICE_TYPE_ID_EV3_MEDIUM_MOTOR,                /**< MINDSTORMS EV3 Medium Motor */

    // Generic & Custom devices
    LEGO_DEVICE_TYPE_ID_NXT_ANALOG,                      /**< MINDSTORMS NXT-style Analog Sensor */
    LEGO_DEVICE_TYPE_ID_NXT_I2C,                         /**< MINDSTORMS NXT-style I2C Sensor */
    LEGO_DEVICE_TYPE_ID_CUSTOM_I2C,                      /**< Custom I2C Sensor */
    LEGO_DEVICE_TYPE_ID_CUSTOM_UART,                     /**< Custom UART Sensor */

    // Categories to match multiple devices
    LEGO_DEVICE_TYPE_ID_ANY_LUMP_UART,                   /**< Any device with LEGO UART Messaging Protocol */
    LEGO_DEVICE_TYPE_ID_ANY_DC_MOTOR,                    /**< Any DC motor */
    LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR,               /**< Any motor with rotation sensors */
} lego_device_type_id_t;

/**
 * Modes for MINDSTORMS EV3 Touch Sensor
 */
enum {
    LEGO_DEVICE_MODE_EV3_TOUCH_SENSOR__TOUCH        = 0,
};

/**
 * Modes for MINDSTORMS EV3 Color Sensor
 */
enum {
    LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__REFLECT      = 0,
    LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__AMBIENT      = 1,
    LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__COLOR        = 2,
    LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__REF_RAW      = 3,
    LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW      = 4,
    LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__CAL          = 5,
};

/**
 * Modes for MINDSTORMS EV3 Infrared Sensor
 */
enum {
    LEGO_DEVICE_MODE_EV3_INFRARED_SENSOR__PROX      = 0,
    LEGO_DEVICE_MODE_EV3_INFRARED_SENSOR__SEEK      = 1,
    LEGO_DEVICE_MODE_EV3_INFRARED_SENSOR__REMOTE    = 2,
    LEGO_DEVICE_MODE_EV3_INFRARED_SENSOR__REM_A     = 3,
    LEGO_DEVICE_MODE_EV3_INFRARED_SENSOR__S_ALT     = 4,
    LEGO_DEVICE_MODE_EV3_INFRARED_SENSOR__CAL       = 5,
};

/**
 * Modes for MINDSTORMS EV3 Ultrasonic Sensor
 */
enum {
    LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM = 0,
    LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__DIST_IN = 1,
    LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__LISTEN  = 2,
    LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__SI_CM   = 3,
    LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__SI_IN   = 4,
    LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__DC_CM   = 5,
    LEGO_DEVICE_MODE_EV3_ULTRASONIC_SENSOR__DC_IN   = 6,
};

/**
 * Modes for MINDSTORMS EV3 Gyro Sensor
 */
enum {
    LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__ANG           = 0,
    LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__RATE          = 1,
    LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__FAS           = 2,
    LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A           = 3,
    LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__CAL           = 4,
    LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__RATE2         = 5,
    LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__ANG2          = 6,
};

/**
 * Modes for MINDSTORMS EV3 Analog Sensor
 */
enum {
    LEGO_DEVICE_MODE_EV3_ANALOG__RAW                = 0,
};

/**
 * Modes for MINDSTORMS NXT Analog Sensor
 */
enum {
    LEGO_DEVICE_MODE_NXT_ANALOG__PASSIVE            = 0,
    LEGO_DEVICE_MODE_NXT_ANALOG__ACTIVE             = 1,
};

/**
 * Modes for MINDSTORMS NXT Ultrasonic Sensor
 */
enum {
    LEGO_DEVICE_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM = 0,
    LEGO_DEVICE_MODE_NXT_ULTRASONIC_SENSOR__DIST_IN = 1,
    LEGO_DEVICE_MODE_NXT_ULTRASONIC_SENSOR__SI_CM   = 2,
    LEGO_DEVICE_MODE_NXT_ULTRASONIC_SENSOR__SI_IN   = 3,
    LEGO_DEVICE_MODE_NXT_ULTRASONIC_SENSOR__LISTEN  = 4,
};

/**
 * Modes for MINDSTORMS NXT Light Sensor
 */
enum {
    LEGO_DEVICE_MODE_NXT_LIGHT_SENSOR__REFLECT      = 0,
    LEGO_DEVICE_MODE_NXT_LIGHT_SENSOR__AMBIENT      = 1,
};

/**
 * Modes for MINDSTORMS NXT Color Sensor
 */
enum {
    LEGO_DEVICE_MODE_NXT_COLOR_SENSOR__MEASURE      = 0,
    LEGO_DEVICE_MODE_NXT_COLOR_SENSOR__LAMP_R       = 1,
    LEGO_DEVICE_MODE_NXT_COLOR_SENSOR__LAMP_G       = 2,
    LEGO_DEVICE_MODE_NXT_COLOR_SENSOR__LAMP_B       = 3,
    LEGO_DEVICE_MODE_NXT_COLOR_SENSOR__LAMP_OFF     = 4,
};

/**
 * Modes for MINDSTORMS NXT Temperature Sensor
 */
enum {
    LEGO_DEVICE_MODE_NXT_TEMPERATURE_SENSOR_CELSIUS = 0,
};

/**
 * Modes for MINDSTORMS NXT Energy Meter
 */
enum {
    LEGO_DEVICE_MODE_NXT_ENERGY_METER_ALL           = 7,
};

/**
 * Modes for POWERED UP WEDO 2.0 Tilt sensor
 */
enum {
    LEGO_DEVICE_MODE_PUP_WEDO2_TILT_SENSOR__ANGLE  = 0, // read 2x int8_t
    LEGO_DEVICE_MODE_PUP_WEDO2_TILT_SENSOR__DIR    = 1, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_WEDO2_TILT_SENSOR__CNT    = 2, // read 3x int8_t
    LEGO_DEVICE_MODE_PUP_WEDO2_TILT_SENSOR__CAL    = 3, // read 3x int8_t
};

/**
 * Modes for POWERED UP WEDO 2.0 Infrared "Motion" sensor
 */
enum {
    LEGO_DEVICE_MODE_PUP_WEDO2_MOTION_SENSOR__DETECT = 0, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT  = 1, // read 1x int32_t
    LEGO_DEVICE_MODE_PUP_WEDO2_MOTION_SENSOR__CAL    = 2, // read 3x int16_t
};

/**
 * Modes for POWERED UP Color and Distance Sensor
 */
enum {
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__COLOR = 0, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX  = 1, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__COUNT = 2, // read 1x int32_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__REFLT = 3, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI  = 4, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__COL_O = 5, // writ 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I = 6, // read 3x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX = 7, // writ 1x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1 = 8, // rrwr 4x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__DEBUG = 9, // ?? 2x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__CALIB = 10, // ?? 8x int16_t
};

/**
 * Modes for SPIKE Color Sensor
 */
enum {
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__COLOR = 0, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__REFLT = 1, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__AMBI  = 2, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__LIGHT = 3, // writ 3x int8_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RREFL = 4, // read 2x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I = 5, // read 4x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__HSV   = 6, // read 3x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__SHSV  = 7, // read 4x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__DEBUG = 8, // ??   2x int16_t
    LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__CALIB = 9, // ??   7x int16_t
};

/**
 * Modes for SPIKE Ultrasonic Sensor
 */
enum {
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__DISTL = 0, // read 1x int16_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__DISTS = 1, // read 1x int16_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__SINGL = 2, // read 1x int16_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__LISTN = 3, // read 1x int8_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__TRAW  = 4, // read 1x int32_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__LIGHT = 5, // writ 4x int8_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__PING  = 6, // ??   1x int8_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__ADRAW = 7, // read 1x int16_t
    LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__CALIB = 8, // ??   7x int16_t
};

/**
 * Modes for SPIKE Force Sensor
 */
enum {
    LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__FRAW  = 4, // read 1x int16_t
    LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__CALIB = 6, // ??   8x int16_t
};

/**
 * Modes for Technic Color Light Matrix
 */
enum {
    LEGO_DEVICE_MODE_PUP_COLOR_LIGHT_MATRIX__LEV_O = 0,
    LEGO_DEVICE_MODE_PUP_COLOR_LIGHT_MATRIX__COL_O = 1,
    LEGO_DEVICE_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O = 2,
    LEGO_DEVICE_MODE_PUP_COLOR_LIGHT_MATRIX__TRANS = 3,
};

/**
 * Modes for Powered Up motors with relative encoders
 */
enum {
    LEGO_DEVICE_MODE_PUP_REL_MOTOR__POWER = 0,
    LEGO_DEVICE_MODE_PUP_REL_MOTOR__SPEED = 1,
    LEGO_DEVICE_MODE_PUP_REL_MOTOR__POS   = 2,
    LEGO_DEVICE_MODE_PUP_REL_MOTOR__TEST  = 3,
};

/**
 * Modes for Powered Up motors with absolute encoders
 */
enum {
    LEGO_DEVICE_MODE_PUP_ABS_MOTOR__POWER = 0,
    LEGO_DEVICE_MODE_PUP_ABS_MOTOR__SPEED = 1,
    LEGO_DEVICE_MODE_PUP_ABS_MOTOR__POS   = 2,
    LEGO_DEVICE_MODE_PUP_ABS_MOTOR__APOS  = 3,
    LEGO_DEVICE_MODE_PUP_ABS_MOTOR__CALIB = 4,
    LEGO_DEVICE_MODE_PUP_ABS_MOTOR__STATS = 5,
};

uint32_t lego_device_stale_data_delay(lego_device_type_id_t id, uint8_t mode);

uint32_t lego_device_data_set_delay(lego_device_type_id_t id, uint8_t mode);

#endif // _LEGO_DEVICES_H_

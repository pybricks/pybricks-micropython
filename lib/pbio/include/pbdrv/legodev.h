// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

/**
 * @addtogroup legodev Driver: Driver for pluggable LEGO devices
 * @{
 */

#ifndef PBDRV_LEGODEV_H
#define PBDRV_LEGODEV_H

#include <pbdrv/config.h>

#include <stddef.h>

#include <pbio/angle.h>
#include <pbio/port.h>

#include <lego_uart.h>


/**
 * Type identifiers for LEGO devices.
 *
 * UART device type IDs are hard-coded in the device, so those numbers can't
 * change. Other ID numbers are arbitrary.
 */
typedef enum {
    PBDRV_LEGODEV_TYPE_ID_NONE                     = 0,    /**< No device is present */

    // LEGO Powered Up non-UART devices - some of these don't exist in real-life
    PBDRV_LEGODEV_TYPE_ID_LPF2_MMOTOR              = 1,    /**< 45303 Powered Up Medium Motor (aka WeDo 2.0 motor) */
    PBDRV_LEGODEV_TYPE_ID_LPF2_TRAIN               = 2,    /**< Powered Up Train Motor */
    /** @cond INTERNAL */
    PBDRV_LEGODEV_TYPE_ID_LPF2_TURN                = 3,
    PBDRV_LEGODEV_TYPE_ID_LPF2_POWER               = 4,
    PBDRV_LEGODEV_TYPE_ID_LPF2_TOUCH               = 5,
    PBDRV_LEGODEV_TYPE_ID_LPF2_LMOTOR              = 6,
    PBDRV_LEGODEV_TYPE_ID_LPF2_XMOTOR              = 7,
    /** @endcond */
    PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT               = 8,    /**< 88005 Powered Up Lights */
    /** @cond INTERNAL */
    PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT1              = 9,
    PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT2              = 10,
    PBDRV_LEGODEV_TYPE_ID_LPF2_TPOINT              = 11,
    PBDRV_LEGODEV_TYPE_ID_LPF2_EXPLOD              = 12,
    PBDRV_LEGODEV_TYPE_ID_LPF2_3_PART              = 13,
    /** @endcond */
    PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART        = 14,   /**< Temporary ID for UART devices until real ID is read from the device */

    // LEGO EV3 UART devices

    /** MINDSTORMS EV3 Color Sensor */
    PBDRV_LEGODEV_TYPE_ID_EV3_COLOR_SENSOR = LUMP_TYPE_ID_EV3_COLOR_SENSOR,

    /** MINDSTORMS EV3 Ultrasonic Sensor */
    PBDRV_LEGODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR = LUMP_TYPE_ID_EV3_ULTRASONIC_SENSOR,

    /** MINDSTORMS EV3 Gyro Sensor */
    PBDRV_LEGODEV_TYPE_ID_EV3_GYRO_SENSOR = LUMP_TYPE_ID_EV3_GYRO_SENSOR,

    /** MINDSTORMS EV3 Infrared Sensor */
    PBDRV_LEGODEV_TYPE_ID_EV3_IR_SENSOR = LUMP_TYPE_ID_EV3_IR_SENSOR,


    // WeDo 2.0 UART devices

    /** WeDo 2.0 Tilt Sensor */
    PBDRV_LEGODEV_TYPE_ID_WEDO2_TILT_SENSOR = LUMP_TYPE_ID_WEDO2_TILT_SENSOR,

    /** WeDo 2.0 Motion Sensor */
    PBDRV_LEGODEV_TYPE_ID_WEDO2_MOTION_SENSOR = LUMP_TYPE_ID_WEDO2_MOTION_SENSOR,

    PBDRV_LEGODEV_TYPE_ID_WEDO2_GENERIC_SENSOR = 36,


    // BOOST UART devices and motors

    /** BOOST Color and Distance Sensor */
    PBDRV_LEGODEV_TYPE_ID_COLOR_DIST_SENSOR = LUMP_TYPE_ID_COLOR_DIST_SENSOR,

    /** BOOST Interactive Motor */
    PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR = LUMP_TYPE_ID_INTERACTIVE_MOTOR,

    /** BOOST Move Hub built-in Motor */
    PBDRV_LEGODEV_TYPE_ID_MOVE_HUB_MOTOR = 39,

    // Technic motors

    /** Technic Large Motor */
    PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_MOTOR = LUMP_TYPE_ID_TECHNIC_L_MOTOR,

    /** Technic XL Motor */
    PBDRV_LEGODEV_TYPE_ID_TECHNIC_XL_MOTOR = LUMP_TYPE_ID_TECHNIC_XL_MOTOR,


    // SPIKE motors

    /** SPIKE Medium Motor */
    PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR = LUMP_TYPE_ID_SPIKE_M_MOTOR,

    /** SPIKE Large Motor */
    PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR = LUMP_TYPE_ID_SPIKE_L_MOTOR,

    // SPIKE sensors

    /** SPIKE Color Sensor */
    PBDRV_LEGODEV_TYPE_ID_SPIKE_COLOR_SENSOR = LUMP_TYPE_ID_SPIKE_COLOR_SENSOR,

    /** SPIKE Ultrasonic Sensor */
    PBDRV_LEGODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR = LUMP_TYPE_ID_SPIKE_ULTRASONIC_SENSOR,

    /** SPIKE Prime Force Sensor */
    PBDRV_LEGODEV_TYPE_ID_SPIKE_FORCE_SENSOR = LUMP_TYPE_ID_SPIKE_FORCE_SENSOR,

    /** Technic Color Light Matrix */
    PBDRV_LEGODEV_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX = LUMP_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX,

    /** SPIKE Small Motor */
    PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR = LUMP_TYPE_ID_SPIKE_S_MOTOR,

    // Technic Angular Motors
    PBDRV_LEGODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR = LUMP_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR,

    PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR = LUMP_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR,

    // NXT Devices
    PBDRV_LEGODEV_TYPE_ID_NXT_TOUCH_SENSOR,                /**< MINDSTORMS NXT Touch Sensor */
    PBDRV_LEGODEV_TYPE_ID_NXT_LIGHT_SENSOR,                /**< MINDSTORMS NXT Light Sensor */
    PBDRV_LEGODEV_TYPE_ID_NXT_SOUND_SENSOR,                /**< MINDSTORMS NXT Sound Sensor */
    PBDRV_LEGODEV_TYPE_ID_NXT_COLOR_SENSOR,                /**< MINDSTORMS NXT Color Sensor */
    PBDRV_LEGODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR,           /**< MINDSTORMS NXT Ultrasonic Sensor */
    PBDRV_LEGODEV_TYPE_ID_NXT_TEMPERATURE_SENSOR,          /**< MINDSTORMS NXT Ultrasonic Sensor */
    PBDRV_LEGODEV_TYPE_ID_NXT_ENERGY_METER,                /**< MINDSTORMS NXT Energy Meter */

    // EV3 Devices
    PBDRV_LEGODEV_TYPE_ID_EV3_TOUCH_SENSOR,                /**< MINDSTORMS EV3 Touch Sensor */
    PBDRV_LEGODEV_TYPE_ID_EV3_LARGE_MOTOR,                 /**< MINDSTORMS EV3 Large Motor */
    PBDRV_LEGODEV_TYPE_ID_EV3_MEDIUM_MOTOR,                /**< MINDSTORMS EV3 Medium Motor */

    // ev3dev devices
    PBDRV_LEGODEV_TYPE_ID_EV3DEV_DC_MOTOR,                 /**< generic ev3dev rcx-motor */
    PBDRV_LEGODEV_TYPE_ID_EV3DEV_LEGO_SENSOR,              /**< generic ev3dev-supported sensor */

    // Generic & Custom devices
    PBDRV_LEGODEV_TYPE_ID_NXT_ANALOG,                      /**< MINDSTORMS NXT-style Analog Sensor */
    PBDRV_LEGODEV_TYPE_ID_NXT_I2C,                         /**< MINDSTORMS NXT-style I2C Sensor */
    PBDRV_LEGODEV_TYPE_ID_CUSTOM_I2C,                      /**< Custom I2C Sensor */
    PBDRV_LEGODEV_TYPE_ID_CUSTOM_UART,                     /**< Custom UART Sensor */

    // Categories to match multiple devices
    PBDRV_LEGODEV_TYPE_ID_ANY_LUMP_UART,                   /**< Any device with LEGO UART Messaging Protocol */
    PBDRV_LEGODEV_TYPE_ID_ANY_DC_MOTOR,                    /**< Any DC motor */
    PBDRV_LEGODEV_TYPE_ID_ANY_ENCODED_MOTOR,               /**< Any motor with rotation sensors */
} pbdrv_legodev_type_id_t;

/**
 * Modes for MINDSTORMS EV3 Touch Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_EV3_TOUCH_SENSOR__TOUCH        = 0,
};

/**
 * Modes for MINDSTORMS EV3 Color Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__REFLECT      = 0,
    PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__AMBIENT      = 1,
    PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__COLOR        = 2,
    PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__REF_RAW      = 3,
    PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__RGB_RAW      = 4,
    PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__CAL          = 5,
};

/**
 * Modes for MINDSTORMS EV3 Infrared Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__PROX      = 0,
    PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__SEEK      = 1,
    PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__REMOTE    = 2,
    PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__REM_A     = 3,
    PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__S_ALT     = 4,
    PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__CAL       = 5,
};

/**
 * Modes for MINDSTORMS EV3 Ultrasonic Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM = 0,
    PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_IN = 1,
    PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__LISTEN  = 2,
    PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM   = 3,
    PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_IN   = 4,
    PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__DC_CM   = 5,
    PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__DC_IN   = 6,
};

/**
 * Modes for MINDSTORMS EV3 Gyro Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_EV3_GYRO_SENSOR__ANG           = 0,
    PBDRV_LEGODEV_MODE_EV3_GYRO_SENSOR__RATE          = 1,
    PBDRV_LEGODEV_MODE_EV3_GYRO_SENSOR__FAS           = 2,
    PBDRV_LEGODEV_MODE_EV3_GYRO_SENSOR__G_A           = 3,
    PBDRV_LEGODEV_MODE_EV3_GYRO_SENSOR__CAL           = 4,
    PBDRV_LEGODEV_MODE_EV3_GYRO_SENSOR__RATE2         = 5,
    PBDRV_LEGODEV_MODE_EV3_GYRO_SENSOR__ANG2          = 6,
};

/**
 * Modes for MINDSTORMS EV3 Analog Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_EV3_ANALOG__RAW                = 0,
};

/**
 * Modes for MINDSTORMS NXT Analog Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_NXT_ANALOG__PASSIVE            = 0,
    PBDRV_LEGODEV_MODE_NXT_ANALOG__ACTIVE             = 1,
};

/**
 * Modes for MINDSTORMS NXT Ultrasonic Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM = 0,
    PBDRV_LEGODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_IN = 1,
    PBDRV_LEGODEV_MODE_NXT_ULTRASONIC_SENSOR__SI_CM   = 2,
    PBDRV_LEGODEV_MODE_NXT_ULTRASONIC_SENSOR__SI_IN   = 3,
    PBDRV_LEGODEV_MODE_NXT_ULTRASONIC_SENSOR__LISTEN  = 4,
};

/**
 * Modes for MINDSTORMS NXT Light Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_NXT_LIGHT_SENSOR__REFLECT      = 0,
    PBDRV_LEGODEV_MODE_NXT_LIGHT_SENSOR__AMBIENT      = 1,
};

/**
 * Modes for MINDSTORMS NXT Color Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_NXT_COLOR_SENSOR__MEASURE      = 0,
    PBDRV_LEGODEV_MODE_NXT_COLOR_SENSOR__LAMP_R       = 1,
    PBDRV_LEGODEV_MODE_NXT_COLOR_SENSOR__LAMP_G       = 2,
    PBDRV_LEGODEV_MODE_NXT_COLOR_SENSOR__LAMP_B       = 3,
    PBDRV_LEGODEV_MODE_NXT_COLOR_SENSOR__LAMP_OFF     = 4,
};

/**
 * Modes for MINDSTORMS NXT Temperature Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_NXT_TEMPERATURE_SENSOR_CELSIUS = 0,
};

/**
 * Modes for MINDSTORMS NXT Energy Meter
 */
enum {
    PBDRV_LEGODEV_MODE_NXT_ENERGY_METER_ALL           = 7,
};

/**
 * Modes for POWERED UP WEDO 2.0 Tilt sensor
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_WEDO2_TILT_SENSOR__ANGLE  = 0,  // read 2x int8_t
    PBDRV_LEGODEV_MODE_PUP_WEDO2_TILT_SENSOR__DIR    = 1,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_WEDO2_TILT_SENSOR__CNT    = 2,  // read 3x int8_t
    PBDRV_LEGODEV_MODE_PUP_WEDO2_TILT_SENSOR__CAL    = 3,  // read 3x int8_t
};

/**
 * Modes for POWERED UP WEDO 2.0 Infrared "Motion" sensor
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__DETECT = 0,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT  = 1,  // read 1x int32_t
    PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL    = 2,  // read 3x int16_t
};

/**
 * Modes for POWERED UP Color and Distance Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__COLOR = 0,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX  = 1,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__COUNT = 2,  // read 1x int32_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__REFLT = 3,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI  = 4,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__COL_O = 5,  // writ 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I = 6,  // read 3x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX = 7,  // writ 1x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1 = 8,  // rrwr 4x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__DEBUG = 9,  // ?? 2x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__CALIB = 10, // ?? 8x int16_t
};

/**
 * Modes for SPIKE Color Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__COLOR = 0,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__REFLT = 1,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__AMBI  = 2,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__LIGHT = 3,  // writ 3x int8_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__RREFL = 4,  // read 2x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__RGB_I = 5,  // read 4x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__HSV   = 6,  // read 3x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__SHSV  = 7,  // read 4x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__DEBUG = 8,  // ??   2x int16_t
    PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__CALIB = 9,  // ??   7x int16_t
};

/**
 * Modes for SPIKE Ultrasonic Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL = 0,  // read 1x int16_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTS = 1,  // read 1x int16_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__SINGL = 2,  // read 1x int16_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__LISTN = 3,  // read 1x int8_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__TRAW  = 4,  // read 1x int32_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__LIGHT = 5,  // writ 4x int8_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__PING  = 6,  // ??   1x int8_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__ADRAW = 7,  // read 1x int16_t
    PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__CALIB = 8,  // ??   7x int16_t
};

/**
 * Modes for SPIKE Force Sensor
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__FRAW  = 4,  // read 1x int16_t
    PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__CALIB = 6,  // ??   8x int16_t
};

/**
 * Modes for Technic Color Light Matrix
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_COLOR_LIGHT_MATRIX__LEV_O = 0,
    PBDRV_LEGODEV_MODE_PUP_COLOR_LIGHT_MATRIX__COL_O = 1,
    PBDRV_LEGODEV_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O = 2,
    PBDRV_LEGODEV_MODE_PUP_COLOR_LIGHT_MATRIX__TRANS = 3,
};

/**
 * Modes for Powered Up motors with relative encoders
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_REL_MOTOR__POWER = 0,
    PBDRV_LEGODEV_MODE_PUP_REL_MOTOR__SPEED = 1,
    PBDRV_LEGODEV_MODE_PUP_REL_MOTOR__POS   = 2,
    PBDRV_LEGODEV_MODE_PUP_REL_MOTOR__TEST  = 3,
};

/**
 * Modes for Powered Up motors with absolute encoders
 */
enum {
    PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__POWER = 0,
    PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__SPEED = 1,
    PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__POS   = 2,
    PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__APOS  = 3,
    PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__CALIB = 4,
    PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__STATS = 5,
};

/**
 * Data types used by I/O devices.
 */
typedef enum {
    /**
     * Signed 8-bit integer.
     */
    PBDRV_LEGODEV_DATA_TYPE_INT8 = LUMP_DATA_TYPE_DATA8,
    /**
     * Little-endian, signed 16-bit integer.
     */
    PBDRV_LEGODEV_DATA_TYPE_INT16 = LUMP_DATA_TYPE_DATA16,
    /**
     * Little-endian, signed 32-bit integer.
     */
    PBDRV_LEGODEV_DATA_TYPE_INT32 = LUMP_DATA_TYPE_DATA32,
    /**
     * Little endian 32-bit floating point.
     */
    PBDRV_LEGODEV_DATA_TYPE_FLOAT = LUMP_DATA_TYPE_DATAF,
} pbdrv_legodev_data_type_t;

/**
 * The maximum number of modes a I/O device can have.
 */
#define PBDRV_LEGODEV_MAX_NUM_MODES    (LUMP_MAX_EXT_MODE + 1)

/**
 * Max size of mode name (not including null terminator)
 */
#define PBDRV_LEGODEV_MODE_NAME_SIZE   LUMP_MAX_NAME_SIZE

/**
 * Max number of data bytes for I/O data. This means that 32 8-bit values, 16
 * 16-bit values or 8 32-bit values are possible.
 */
#define PBDRV_LEGODEV_MAX_DATA_SIZE    LUMP_MAX_MSG_SIZE

/**
 * I/O device capability flags.
 */
typedef enum {
    /**
     * Convience value for no flags set.
     */
    PBDRV_LEGODEV_CAPABILITY_FLAG_NONE = 0,
    /**
     * Indicates that this device is a DC output such as a motor or a light.
     */
    PBDRV_LEGODEV_CAPABILITY_FLAG_IS_DC_OUTPUT = 1 << 0,
    /**
     * Indicates that the motor provides speed feedback.
     */
    PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_SPEED = 1 << 1,
    /**
     * Indicates that the motor provides relative position feedback.
     */
    PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS = 1 << 2,
    /**
     * Indicates that the motor provides absolute position feedback.
     */
    PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS = 1 << 3,
    /**
     * Indicates that the device requires power supply across pin 1 (+) and pin 2 (-).
     */
    PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1 = 1 << 4,
    /**
     * Indicates that the device requires power supply across pin 1 (-) and pin 2 (+).
     */
    PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2 = 1 << 5,
} pbdrv_legodev_capability_flags_t;

/**
 * Opaque handle to a legodev device instance.
 */
typedef struct _pbdrv_legodev_dev_t pbdrv_legodev_dev_t;

/**
 * Structure containing information about a legodev device mode.
 */
typedef struct {
    /**< The number of values returned by the current mode. */
    uint8_t num_values;
    /**< The data type of the values returned by the current mode. */
    pbdrv_legodev_data_type_t data_type;
    /**< Whether the mode supports setting data */
    bool writable;
    /**< Zero-terminated name of the mode. */
    char name[LUMP_MAX_NAME_SIZE + 1];
} pbdrv_legodev_mode_info_t;

/**
 * Structure containing information about a legodev device.
 */
typedef struct {
    /**< The type identifier of the device. */
    pbdrv_legodev_type_id_t type_id;
    /**< The capabilities and requirements of the device. */
    pbdrv_legodev_capability_flags_t flags;
    /**< The current mode of the device. */
    uint8_t mode;
    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    /**< The number of modes */
    uint8_t num_modes;
    /**< Information about the current mode. */
    pbdrv_legodev_mode_info_t mode_info[PBDRV_LEGODEV_MAX_NUM_MODES];
    #endif
} pbdrv_legodev_info_t;

#if PBDRV_CONFIG_LEGODEV

/**
 * Gets a legodev device instance if it is attached.
 *
 * @param [in]     port_id   The requested port.
 * @param [in,out] type_id   The expected/desired device type or category. Outputs what is actually there.
 * @param [out]    legodev   The legodev device instance.
 * @return                   ::PBIO_SUCCESS if the expected device was there.
 *                           ::PBIO_ERROR_NO_DEV if expected device was not there.
 *                           ::PBIO_ERROR_AGAIN if this should be called again to proceed.
 */
pbio_error_t pbdrv_legodev_get_device(pbio_port_id_t port_id, pbdrv_legodev_type_id_t *type_id, pbdrv_legodev_dev_t **legodev);

/**
 * Gets information about the legodev device.
 *
 * @param [in]  legodev   The legodev device instance.
 * @param [out] info      The legodev device information.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device is attached or does not support it.
 */
pbio_error_t pbdrv_legodev_get_info(pbdrv_legodev_dev_t *legodev, pbdrv_legodev_info_t **info);

/**
 * Checks if the legodev device is ready for reading or writing data.
 *
 * @param [in]  legodev   The legodev device instance.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device is attached or does not support it.
 */
pbio_error_t pbdrv_legodev_is_ready(pbdrv_legodev_dev_t *legodev);

/**
 * Starts setting the mode of the legodev device.
 *
 * @param [in]  legodev   The legodev device instance.
 * @param [in]  mode      The mode to set.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device is attached or does not support it.
 */
pbio_error_t pbdrv_legodev_set_mode(pbdrv_legodev_dev_t *legodev, uint8_t mode);

/**
 * Sets the mode of the legodev device with additional data.
 *
 * @param [in]  legodev   The legodev device instance.
 * @param [in]  mode      The mode to set.
 * @param [in]  data      The additional data.
 * @param [in]  size      The size of the additional data.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device is attached or does not support it.
 */
pbio_error_t pbdrv_legodev_set_mode_with_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, const void *data, uint8_t size);

/**
 * Gets data from the legodev device.
 *
 * @param [in]  legodev   The legodev device instance.
 * @param [in]  mode      The mode to get data for.
 * @param [out] data      The data, aligned at 4 bytes.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device is attached or does not support it.
 */
pbio_error_t pbdrv_legodev_get_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, void **data);

// The following functions are used only by other pbdrv drivers.

/**
 * Gets the motor index of the legodev device.
 *
 * @param [in]  legodev   The legodev device instance.
 * @param [out] index     The motor index.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if this device does not have a motor port.
 */
pbio_error_t pbdrv_legodev_get_motor_index(pbdrv_legodev_dev_t *legodev, uint8_t *index);

/**
 * Gets the angle of the legodev device.
 *
 * @param [in]  legodev   The legodev device instance.
 * @param [out] angle     The angle.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device is attached or does not support it.
 */
pbio_error_t pbdrv_legodev_get_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle);

/**
 * Gets the absolute angle of the legodev device.
 *
 * @param [in]  legodev   The legodev device instance.
 * @param [out] angle     The absolute angle.
 * @return                ::PBIO_SUCCESS on success.
 *                        ::PBIO_ERROR_NO_DEV if no device is attached or does not support it.
 */
pbio_error_t pbdrv_legodev_get_abs_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle);

/**
 * Checks if the legodev device needs permanent power.
 *
 * This is used to prevent turning off power when the program ends.
 *
 * @param [in]  legodev   The legodev device instance.
 * @return                True if the device needs permanent power, false otherwise.
 */
bool pbdrv_legodev_needs_permanent_power(pbdrv_legodev_dev_t *legodev);

#else // PBDRV_CONFIG_LEGODEV

static inline pbio_error_t pbdrv_legodev_get_device(pbio_port_id_t port_id, pbdrv_legodev_type_id_t *type_id, pbdrv_legodev_dev_t **legodev) {
    return PBIO_ERROR_NO_DEV;
}

static inline pbio_error_t pbdrv_legodev_get_motor_index(pbdrv_legodev_dev_t *legodev, uint8_t *index) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_legodev_get_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_legodev_get_abs_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline bool pbdrv_legodev_needs_permanent_power(pbdrv_legodev_dev_t *legodev) {
    return false;
}

static inline pbio_error_t pbdrv_legodev_get_info(pbdrv_legodev_dev_t *legodev, pbdrv_legodev_info_t **info) {
    *info = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_legodev_is_ready(pbdrv_legodev_dev_t *legodev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_legodev_set_mode(pbdrv_legodev_dev_t *legodev, uint8_t mode) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_legodev_set_mode_with_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, const void *data, uint8_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_legodev_get_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, void **data) {
    *data = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_LEGODEV

#endif // PBDRV_LEGODEV_H

/** @} */

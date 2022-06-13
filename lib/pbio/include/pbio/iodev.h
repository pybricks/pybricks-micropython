// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_IODEV_H_
#define _PBIO_IODEV_H_

#include <stddef.h>
#include <stdint.h>

#include <lego_uart.h>

#include "pbio/port.h"

/**
 * Type identifiers for I/O devices.
 *
 * UART device type IDs are hard-coded in the device, so those numbers can't
 * change. Other ID numbers are arbitrary.
 */
typedef enum {
    PBIO_IODEV_TYPE_ID_NONE                     = 0,    /**< No device is present */

    // LEGO Powered Up non-UART devices - some of these don't exist in real-life
    PBIO_IODEV_TYPE_ID_LPF2_MMOTOR              = 1,    /**< 45303 Powered Up Medium Motor (aka WeDo 2.0 motor) */
    PBIO_IODEV_TYPE_ID_LPF2_TRAIN               = 2,    /**< Powered Up Train Motor */
    /** @cond INTERNAL */
    PBIO_IODEV_TYPE_ID_LPF2_TURN                = 3,
    PBIO_IODEV_TYPE_ID_LPF2_POWER               = 4,
    PBIO_IODEV_TYPE_ID_LPF2_TOUCH               = 5,
    PBIO_IODEV_TYPE_ID_LPF2_LMOTOR              = 6,
    PBIO_IODEV_TYPE_ID_LPF2_XMOTOR              = 7,
    /** @endcond */
    PBIO_IODEV_TYPE_ID_LPF2_LIGHT               = 8,    /**< 88005 Powered Up Lights */
    /** @cond INTERNAL */
    PBIO_IODEV_TYPE_ID_LPF2_LIGHT1              = 9,
    PBIO_IODEV_TYPE_ID_LPF2_LIGHT2              = 10,
    PBIO_IODEV_TYPE_ID_LPF2_TPOINT              = 11,
    PBIO_IODEV_TYPE_ID_LPF2_EXPLOD              = 12,
    PBIO_IODEV_TYPE_ID_LPF2_3_PART              = 13,
    /** @endcond */
    PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART        = 14,   /**< Temporary ID for UART devices until real ID is read from the device */

    // LEGO EV3 UART devices

    /** MINDSTORMS EV3 Color Sensor */
    PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR = LUMP_TYPE_ID_EV3_COLOR_SENSOR,

    /** MINDSTORMS EV3 Ultrasonic Sensor */
    PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR = LUMP_TYPE_ID_EV3_ULTRASONIC_SENSOR,

    /** MINDSTORMS EV3 Gyro Sensor */
    PBIO_IODEV_TYPE_ID_EV3_GYRO_SENSOR = LUMP_TYPE_ID_EV3_GYRO_SENSOR,

    /** MINDSTORMS EV3 Infrared Sensor */
    PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR = LUMP_TYPE_ID_EV3_IR_SENSOR,


    // WeDo 2.0 UART devices

    /** WeDo 2.0 Tilt Sensor */
    PBIO_IODEV_TYPE_ID_WEDO2_TILT_SENSOR = LUMP_TYPE_ID_WEDO2_TILT_SENSOR,

    /** WeDo 2.0 Motion Sensor */
    PBIO_IODEV_TYPE_ID_WEDO2_MOTION_SENSOR = LUMP_TYPE_ID_WEDO2_MOTION_SENSOR,

    PBIO_IODEV_TYPE_ID_WEDO2_GENERIC_SENSOR = 36,


    // BOOST UART devices and motors

    /** BOOST Color and Distance Sensor */
    PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR = LUMP_TYPE_ID_COLOR_DIST_SENSOR,

    /** BOOST Interactive Motor */
    PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR = LUMP_TYPE_ID_INTERACTIVE_MOTOR,

    /** BOOST Move Hub built-in Motor */
    PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR = 39,

    // Technic motors

    /** Technic Large Motor */
    PBIO_IODEV_TYPE_ID_TECHNIC_L_MOTOR = LUMP_TYPE_ID_TECHNIC_L_MOTOR,

    /** Technic XL Motor */
    PBIO_IODEV_TYPE_ID_TECHNIC_XL_MOTOR = LUMP_TYPE_ID_TECHNIC_XL_MOTOR,


    // SPIKE motors

    /** SPIKE Medium Motor */
    PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR = LUMP_TYPE_ID_SPIKE_M_MOTOR,

    /** SPIKE Large Motor */
    PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR = LUMP_TYPE_ID_SPIKE_L_MOTOR,

    // SPIKE sensors

    /** SPIKE Color Sensor */
    PBIO_IODEV_TYPE_ID_SPIKE_COLOR_SENSOR = LUMP_TYPE_ID_SPIKE_COLOR_SENSOR,

    /** SPIKE Ultrasonic Sensor */
    PBIO_IODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR = LUMP_TYPE_ID_SPIKE_ULTRASONIC_SENSOR,

    /** SPIKE Prime Force Sensor */
    PBIO_IODEV_TYPE_ID_SPIKE_FORCE_SENSOR = LUMP_TYPE_ID_SPIKE_FORCE_SENSOR,

    /** Technic Color Light Matrix */
    PBIO_IODEV_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX = LUMP_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX,

    /** SPIKE Small Motor */
    PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR = LUMP_TYPE_ID_SPIKE_S_MOTOR,

    // Technic Angular Motors
    PBIO_IODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR = LUMP_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR,

    PBIO_IODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR = LUMP_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR,

    // NXT Devices
    PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR,                /**< MINDSTORMS NXT Touch Sensor */
    PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR,                /**< MINDSTORMS NXT Light Sensor */
    PBIO_IODEV_TYPE_ID_NXT_SOUND_SENSOR,                /**< MINDSTORMS NXT Sound Sensor */
    PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR,                /**< MINDSTORMS NXT Color Sensor */
    PBIO_IODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR,           /**< MINDSTORMS NXT Ultrasonic Sensor */
    PBIO_IODEV_TYPE_ID_NXT_TEMPERATURE_SENSOR,          /**< MINDSTORMS NXT Ultrasonic Sensor */
    PBIO_IODEV_TYPE_ID_NXT_ENERGY_METER,                /**< MINDSTORMS NXT Energy Meter */

    // EV3 Devices
    PBIO_IODEV_TYPE_ID_EV3_TOUCH_SENSOR,                /**< MINDSTORMS EV3 Touch Sensor */
    PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR,                 /**< MINDSTORMS EV3 Large Motor */
    PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR,                /**< MINDSTORMS EV3 Medium Motor */

    // ev3dev devices
    PBIO_IODEV_TYPE_ID_EV3DEV_DC_MOTOR,                 /**< generic ev3dev rcx-motor */
    PBIO_IODEV_TYPE_ID_EV3DEV_LEGO_SENSOR,              /**< generic ev3dev-supported sensor */

    // Generic & Custom devices
    PBIO_IODEV_TYPE_ID_NXT_ANALOG,                      /**< MINDSTORMS NXT-style Analog Sensor */
    PBIO_IODEV_TYPE_ID_NXT_I2C,                         /**< MINDSTORMS NXT-style I2C Sensor */
    PBIO_IODEV_TYPE_ID_CUSTOM_I2C,                      /**< Custom I2C Sensor */
    PBIO_IODEV_TYPE_ID_LUMP_UART,                       /**< Device with LEGO UART Messaging Protocol */
    PBIO_IODEV_TYPE_ID_CUSTOM_UART,                     /**< Custom UART Sensor */

} pbio_iodev_type_id_t;

/**
 * Modes for MINDSTORMS EV3 Touch Sensor
 */
enum {
    PBIO_IODEV_MODE_EV3_TOUCH_SENSOR__TOUCH        = 0,
};

/**
 * Modes for MINDSTORMS EV3 Color Sensor
 */
enum {
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__REFLECT      = 0,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__AMBIENT      = 1,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__COLOR        = 2,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__REF_RAW      = 3,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__RGB_RAW      = 4,
    PBIO_IODEV_MODE_EV3_COLOR_SENSOR__CAL          = 5,
};

/**
 * Modes for MINDSTORMS EV3 Infrared Sensor
 */
enum {
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__PROX      = 0,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__SEEK      = 1,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REMOTE    = 2,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REM_A     = 3,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__S_ALT     = 4,
    PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__CAL       = 5,
};

/**
 * Modes for MINDSTORMS EV3 Ultrasonic Sensor
 */
enum {
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM = 0,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_IN = 1,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__LISTEN  = 2,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM   = 3,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_IN   = 4,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DC_CM   = 5,
    PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DC_IN   = 6,
};

/**
 * Modes for MINDSTORMS EV3 Gyro Sensor
 */
enum {
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__ANG           = 0,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__RATE          = 1,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__FAS           = 2,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__G_A           = 3,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__CAL           = 4,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__RATE2         = 5,
    PBIO_IODEV_MODE_EV3_GYRO_SENSOR__ANG2          = 6,
};

/**
 * Modes for MINDSTORMS EV3 Analog Sensor
 */
enum {
    PBIO_IODEV_MODE_EV3_ANALOG__RAW                = 0,
};

/**
 * Modes for MINDSTORMS NXT Analog Sensor
 */
enum {
    PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE            = 0,
    PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE             = 1,
};

/**
 * Modes for MINDSTORMS NXT Ultrasonic Sensor
 */
enum {
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM = 0,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_IN = 1,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__SI_CM   = 2,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__SI_IN   = 3,
    PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__LISTEN  = 4,
};

/**
 * Modes for MINDSTORMS NXT Light Sensor
 */
enum {
    PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__REFLECT      = 0,
    PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__AMBIENT      = 1,
};

/**
 * Modes for MINDSTORMS NXT Color Sensor
 */
enum {
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE      = 0,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_R       = 1,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_G       = 2,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_B       = 3,
    PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_OFF     = 4,
};

/**
 * Modes for MINDSTORMS NXT Temperature Sensor
 */
enum {
    PBIO_IODEV_MODE_NXT_TEMPERATURE_SENSOR_CELCIUS = 0,
};

/**
 * Modes for MINDSTORMS NXT Energy Meter
 */
enum {
    PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL           = 7,
};

/**
 * Modes for POWERED UP WEDO 2.0 Tilt sensor
 */
enum {
    PBIO_IODEV_MODE_PUP_WEDO2_TILT_SENSOR__ANGLE  = 0,  // read 2x int8_t
    PBIO_IODEV_MODE_PUP_WEDO2_TILT_SENSOR__DIR    = 1,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_WEDO2_TILT_SENSOR__CNT    = 2,  // read 3x int8_t
    PBIO_IODEV_MODE_PUP_WEDO2_TILT_SENSOR__CAL    = 2,  // read 3x int8_t
};

/**
 * Modes for POWERED UP WEDO 2.0 Infrared "Motion" sensor
 */
enum {
    PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__DETECT = 0,  // read 1x int8_t
    PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT  = 1,  // read 1x int32_t
    PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL    = 2,  // read 3x int16_t
};

/**
 * Modes for POWERED UP Color and Distance Sensor
 */
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

/**
 * Modes for SPIKE Color Sensor
 */
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

/**
 * Modes for SPIKE Ultrasonic Sensor
 */
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

/**
 * Modes for SPIKE Force Sensor
 */
enum {
    PBIO_IODEV_MODE_PUP_FORCE_SENSOR__FRAW  = 4,  // read 1x int16_t
    PBIO_IODEV_MODE_PUP_FORCE_SENSOR__CALIB = 6,  // ??   8x int16_t
};

/**
 * Modes for Technic Color Light Matrix
 */
enum {
    PBIO_IODEV_MODE_PUP_COLOR_LIGHT_MATRIX__LEV_O = 0,
    PBIO_IODEV_MODE_PUP_COLOR_LIGHT_MATRIX__COL_O = 1,
    PBIO_IODEV_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O = 2,
    PBIO_IODEV_MODE_PUP_COLOR_LIGHT_MATRIX__TRANS = 3,
};

/**
 * Modes for Powered Up motors with relative encoders
 */
enum {
    PBIO_IODEV_MODE_PUP_REL_MOTOR__POWER = 0,
    PBIO_IODEV_MODE_PUP_REL_MOTOR__SPEED = 1,
    PBIO_IODEV_MODE_PUP_REL_MOTOR__POS   = 2,
    PBIO_IODEV_MODE_PUP_REL_MOTOR__TEST  = 3,
};

/**
 * Modes for Powered Up motors with absolute encoders
 */
enum {
    PBIO_IODEV_MODE_PUP_ABS_MOTOR__POWER = 0,
    PBIO_IODEV_MODE_PUP_ABS_MOTOR__SPEED = 1,
    PBIO_IODEV_MODE_PUP_ABS_MOTOR__POS   = 2,
    PBIO_IODEV_MODE_PUP_ABS_MOTOR__APOS  = 3,
    PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB = 4,
    PBIO_IODEV_MODE_PUP_ABS_MOTOR__STATS = 5,
};

/**
 * Data types used by I/O devices.
 */
typedef enum {
    /**
     * Signed 8-bit integer.
     */
    PBIO_IODEV_DATA_TYPE_INT8 = LUMP_DATA_TYPE_DATA8,
    /**
     * Little-endian, signed 16-bit integer.
     */
    PBIO_IODEV_DATA_TYPE_INT16 = LUMP_DATA_TYPE_DATA16,
    /**
     * Little-endian, signed 32-bit integer.
     */
    PBIO_IODEV_DATA_TYPE_INT32 = LUMP_DATA_TYPE_DATA32,
    /**
     * Little endian 32-bit floating point.
     */
    PBIO_IODEV_DATA_TYPE_FLOAT = LUMP_DATA_TYPE_DATAF,
} pbio_iodev_data_type_t;

// Bit mask for pbio_iodev_data_type_t
#define PBIO_IODEV_DATA_TYPE_MASK (0x03)

// Third bit indicates whether the mode supports writing
#define PBIO_IODEV_DATA_TYPE_WRITABLE (0x04)

/**
 * The maximum number of modes a I/O device can have.
 */
#define PBIO_IODEV_MAX_NUM_MODES    (LUMP_MAX_EXT_MODE + 1)

/**
 * Max size of mode name (not including null terminator)
 */
#define PBIO_IODEV_MODE_NAME_SIZE   LUMP_MAX_NAME_SIZE

/**
 * Max number of data bytes for I/O data. This means that 32 8-bit values, 16
 * 16-bit values or 8 32-bit values are possible.
 */
#define PBIO_IODEV_MAX_DATA_SIZE    LUMP_MAX_MSG_SIZE

/**
 * Max size of units of measurements (not including null terminator)
 */
#define PBIO_IODEV_UOM_SIZE         LUMP_MAX_UOM_SIZE

/**
 * I/O device capability flags.
 */
typedef enum {
    /**
     * Convience value for no flags set.
     */
    PBIO_IODEV_CAPABILITY_FLAG_NONE = 0,
    /**
     * Indicates that this device is a DC output such as a motor or a light.
     */
    PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT = 1 << 0,
    /**
     * Indicates that the motor provides speed feedback.
     */
    PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_SPEED = 1 << 1,
    /**
     * Indicates that the motor provides relative position feedback.
     */
    PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS = 1 << 2,
    /**
     * Indicates that the motor provides absolute position feedback.
     */
    PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS = 1 << 3,
    /**
     * Indicates that the device requires power supply across pin 1 (+) and pin 2 (-).
     */
    PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1 = 1 << 4,
    /**
     * Indicates that the device requires power supply across pin 1 (-) and pin 2 (+).
     */
    PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2 = 1 << 5,
} pbio_iodev_capability_flags_t;

/**
 * Macro for testing if I/O device is a dc output such as a motor or light.
 *
 * @param [in] d    Pointer to pbio_iodev_t.
 */
#define PBIO_IODEV_IS_DC_OUTPUT(d) ((d)->info->capability_flags & PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT)

/**
 * Macro for testing if I/O device is a motor with speed/position feedback.
 *
 * @param [in] d    Pointer to pbio_iodev_t.
 */
#define PBIO_IODEV_IS_FEEDBACK_MOTOR(d) ((d)->info->capability_flags & PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS)

/**
 * Macro for testing if I/O device is a motor with absolute position feedback.
 *
 * @param [in] d    Pointer to pbio_iodev_t.
 */
#define PBIO_IODEV_IS_ABS_MOTOR(d) ((d)->info->capability_flags & PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS)



/**
 * Mapping flags that describe the input and output values of an I/O device.
 */
typedef enum {
    LPF2_MAPPING_FLAG_UNKNOWN_BIT1 = 1 << 1,
    /** The value is a discrete value, e.g. a color index. */
    LPF2_MAPPING_FLAG_DISCRETE  = 1 << 2,
    /** The value is a relative value, e.g. a motor position. */
    LPF2_MAPPING_FLAG_RELATIVE  = 1 << 3,
    /** The value is an absolute value, e.g. a distance measurement. */
    LPF2_MAPPING_FLAG_ABSOLUTE  = 1 << 4,
    LPF2_MAPPING_FLAG_UNKNOWN_BIT5 = 1 << 5,
    /** Supports functional mapping 2.0+. */
    LPF2_MAPPING_FLAG_2_0       = 1 << 6,
    /** Supports NULL value. */
    LPF2_MAPPING_FLAG_NULL      = 1 << 7,
} pbio_iodev_mapping_flag_t;

/**
 * Information about one mode of an I/O device.
 */
typedef struct {
    /**
     * The number of data values for this mode.
     */
    uint8_t num_values;
    /**
     * The binary format of the data for this mode and writability flag.
     */
    pbio_iodev_data_type_t data_type;
} pbio_iodev_mode_t;

/**
 * Device-specific data for an I/O device.
 */
typedef struct {
    /**
     * The type identifier for the device.
     */
    pbio_iodev_type_id_t type_id;
    /**
     * Device capability flags.
     */
    pbio_iodev_capability_flags_t capability_flags;
    /**
     * The number of modes the device has. (Indicates size of *mode_info*).
     */
    uint8_t num_modes;
    /**
     * Bit flags indicating which combination of modes can be used at the same
     * time. Each bit cooresponds to the mode of the same number (0 to 15).
     */
    uint16_t mode_combos;
    /**
     * Array of mode info for all modes. Array size depends on the device.
     */
    pbio_iodev_mode_t mode_info[0];
} pbio_iodev_info_t;

/**
 * Data structure for holding an I/O device's state.
 */
typedef struct _pbio_iodev_t pbio_iodev_t;

/** @cond INTERNAL */

/**
 * Device-specific communication functions.
 */
typedef struct {
    pbio_error_t (*set_mode_begin)(pbio_iodev_t *iodev, uint8_t mode);
    pbio_error_t (*set_mode_end)(pbio_iodev_t *iodev);
    void (*set_mode_cancel)(pbio_iodev_t *iodev);
    pbio_error_t (*set_data_begin)(pbio_iodev_t *iodev, const uint8_t *data);
    pbio_error_t (*set_data_end)(pbio_iodev_t *iodev);
    void (*set_data_cancel)(pbio_iodev_t *iodev);
    pbio_error_t (*write_begin)(pbio_iodev_t *iodev, const uint8_t *data, uint8_t size);
    pbio_error_t (*write_end)(pbio_iodev_t *iodev);
    void (*write_cancel)(pbio_iodev_t *iodev);
} pbio_iodev_ops_t;

struct _pbio_iodev_t {
    /**
     * Pointer to the mode info for this device.
     */
    const pbio_iodev_info_t *info;
    /**
     * Pointer to the device-specific communication functions.
     */
    const pbio_iodev_ops_t *ops;
    /**
     * The port the device is attached to.
     */
    pbio_port_id_t port;
    /**
     * The current active mode.
     */
    uint8_t mode;
    /**
     * Most recent binary data read from the device. How to interpret this data
     * is determined by the ::pbio_iodev_mode_t info associated with the current
     * *mode* of the device. For example, it could be an array of int32_t and/or
     * the values could be foreign-endian.
     */
    uint8_t bin_data[PBIO_IODEV_MAX_DATA_SIZE]  __attribute__((aligned(4)));
};

/** @endcond */

size_t pbio_iodev_size_of(pbio_iodev_data_type_t type);
pbio_error_t pbio_iodev_get_data_format(pbio_iodev_t *iodev, uint8_t mode, uint8_t *len, pbio_iodev_data_type_t *type);
pbio_error_t pbio_iodev_get_data(pbio_iodev_t *iodev, uint8_t **data);
pbio_error_t pbio_iodev_set_mode_begin(pbio_iodev_t *iodev, uint8_t mode);
pbio_error_t pbio_iodev_set_mode_end(pbio_iodev_t *iodev);
void pbio_iodev_set_mode_cancel(pbio_iodev_t *iodev);
pbio_error_t pbio_iodev_set_data_begin(pbio_iodev_t *iodev, uint8_t mode, const uint8_t *data);
pbio_error_t pbio_iodev_set_data_end(pbio_iodev_t *iodev);
void pbio_iodev_set_data_cancel(pbio_iodev_t *iodev);
pbio_error_t pbio_iodev_write_begin(pbio_iodev_t *iodev, const uint8_t *data, uint8_t size);
pbio_error_t pbio_iodev_write_end(pbio_iodev_t *iodev);
void pbio_iodev_write_cancel(pbio_iodev_t *iodev);

#endif // _PBIO_IODEV_H_

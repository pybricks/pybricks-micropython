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
    PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR            = 48,

    /** SPIKE Large Motor */
    PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR            = 49,

    // SPIKE sensors

    /** SPIKE Color Sensor */
    PBIO_IODEV_TYPE_ID_SPIKE_COLOR_SENSOR       = 61,

    /** SPIKE Ultrasonic Sensor */
    PBIO_IODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR  = 62,

    /** SPIKE Prime Force Sensor */
    PBIO_IODEV_TYPE_ID_SPIKE_FORCE_SENSOR       = 63,

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
    PBIO_IODEV_MOTOR_FLAG_NONE = 0,
    /**
     * Indicates that this device is motor.
     */
    PBIO_IODEV_MOTOR_FLAG_IS_MOTOR = 1 << 0,
    /**
     * Indicates that the motor provides speed feedback.
     */
    PBIO_IODEV_MOTOR_FLAG_HAS_SPEED = 1 << 1,
    /**
     * Indicates that the motor provides relative position feedback.
     */
    PBIO_IODEV_MOTOR_FLAG_HAS_REL_POS = 1 << 2,
    /**
     * Indicates that the motor provides absolute position feedback.
     */
    PBIO_IODEV_MOTOR_FLAG_HAS_ABS_POS = 1 << 3,
} pbio_iodev_motor_flags_t;

/**
 * Macro for testing if I/O device is a motor.
 *
 * @param [in] d    Pointer to pbio_iodev_t.
 */
#define PBIO_IODEV_IS_MOTOR(d) ((d)->motor_flags & PBIO_IODEV_MOTOR_FLAG_IS_MOTOR)

/**
 * Macro for testing if I/O device is a motor with speed/position feedback.
 *
 * @param [in] d    Pointer to pbio_iodev_t.
 */
#define PBIO_IODEV_IS_FEEDBACK_MOTOR(d) ((d)->motor_flags > PBIO_IODEV_MOTOR_FLAG_IS_MOTOR)

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
     * The name of the mode.
     */
    char name[PBIO_IODEV_MODE_NAME_SIZE + 1];
    /**
     * Mode flags.
     */
    lump_mode_flags_t flags;
    /**
     * The number of data values for this mode.
     */
    uint8_t num_values;
    /**
     * The binary format of the data for this mode.
     */
    pbio_iodev_data_type_t data_type;
    /**
     * The number of digits for display, including the decimal point.
     */
    uint8_t digits;
    /**
     * The number of digits after the decimal point.
     */
    uint8_t decimals;
    /**
     * The minimum raw data value. This is just used for scaling an may not
     * actually be the minimum possible value.
     */
    float raw_min;
    /**
     * The maximum raw data value. This is just used for scaling an may not
     * actually be the maximum possible value.
     */
    float raw_max;
    /**
     * The minimum percent data value. This will be either 0.0 or -100.0.
     */
    float pct_min;
    /**
     * The maximum percent data value. This will always be 100.0.
     */
    float pct_max;
    /**
     * The minimum scaled data value. This is just used for scaling an may not
     * actually be the minimum possible value.
     */
    float si_min;
    /**
     * The maximum scaled data value. This is just used for scaling an may not
     * actually be the maximum possible value.
     */
    float si_max;
    /**
     * The units of measurement.
     */
    char uom[PBIO_IODEV_UOM_SIZE + 1];
    /**
     * Input value mapping flags.
     */
    pbio_iodev_mapping_flag_t input_flags;
    /**
     * Output value mapping flags.
     */
    pbio_iodev_mapping_flag_t output_flags;
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
     * The number of modes the device has. (Indicates size of *mode_info*).
     */
    uint8_t num_modes;
    /**
     * The number of "view" modes the device has. This modes are show in the
     * port monitor. Other modes may or may not be useable depending on the
     * device (e.g. some are for factory calibration).
     */
    uint8_t num_view_modes;
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
    pbio_port_t port;
    /**
     * The current active mode.
     */
    uint8_t mode;
    /**
     * Motor capability flags.
     */
    pbio_iodev_motor_flags_t motor_flags;
    /**
     * Most recent binary data read from the device. How to interpret this data
     * is determined by the ::pbio_iodev_mode_t info associated with the current
     * *mode* of the device. For example, it could be an array of int32_t and/or
     * the values could be foreign-endian.
     */
    uint8_t bin_data[PBIO_IODEV_MAX_DATA_SIZE]  __attribute__((aligned(32)));
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

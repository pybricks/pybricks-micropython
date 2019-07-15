// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBIO_IODEV_H_
#define _PBIO_IODEV_H_

#include <stddef.h>
#include <stdint.h>

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
    PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR         = 29,   /**< MINDSTORMS EV3 Color Sensor */
    PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR    = 30,   /**< MINDSTORMS EV3 Ultrasonic Sensor */
    PBIO_IODEV_TYPE_ID_EV3_GYRO_SENSOR          = 32,   /**< MINDSTORMS EV3 Gyro Sensor */
    PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR            = 33,   /**< MINDSTORMS EV3 Infrared Sensor */

    // WeDo 2.0 UART devices
    PBIO_IODEV_TYPE_ID_WEDO2_TILT_SENSOR        = 34,   /**< WeDo 2.0 Tilt Sensor */
    PBIO_IODEV_TYPE_ID_WEDO2_MOTION_SENSOR      = 35,   /**< WeDo 2.0 Motion Sensor */
    PBIO_IODEV_TYPE_ID_WEDO2_GENERIC_SENSOR     = 36,

    // BOOST UART devices and motors
    PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR        = 37,   /**< BOOST Color and Distance Sensor */
    PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR        = 38,   /**< BOOST Interactive Motor */
    PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR           = 39,   /**< BOOST Move Hub built-in Motor */

    // TECHNIC Control+ motors
    PBIO_IODEV_TYPE_ID_CPLUS_L_MOTOR            = 46,   /**< TECHNIC Control+ Large Motor */
    PBIO_IODEV_TYPE_ID_CPLUS_XL_MOTOR           = 47,   /**< TECHNIC Control+ XL Motor */

    // FatcatLab EV3 UART devices
    PBIO_IODEV_TYPE_ID_FCL_ADC                  = 71,   /**< FatcatLab A/DC Adapter */
    PBIO_IODEV_TYPE_ID_FCL_GESTURE              = 72,   /**< FatcatLab Gesture Sensor */
    PBIO_IODEV_TYPE_ID_FCL_LIGHT                = 73,   /**< FatcatLab Light Sensor */
    PBIO_IODEV_TYPE_ID_FCL_ALTITUDE             = 74,   /**< FatcatLab Altitude Sensor */
    PBIO_IODEV_TYPE_ID_FCL_IR                   = 75,   /**< FatcatLab IR Receiver */
    PBIO_IODEV_TYPE_ID_FCL_9DOF                 = 76,   /**< FatcatLab 9DOF Sensor */
    PBIO_IODEV_TYPE_ID_FCL_HUMIDITY             = 77,   /**< FatcatLab Humidity Sensor */

    // LEGO EV3 motors TODO: assign number and location in this table
    PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR              ,   /**< MINDSTORMS EV3 Large Motor */
    PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR             ,   /**< MINDSTORMS EV3 Medium Motor */
    PBIO_IODEV_TYPE_ID_EV3_DC_MOTOR                 ,   /**< ev3dev DC Motor */
} pbio_iodev_type_id_t;

/**
 * Mode identifiers for I/O devices.
 */
typedef enum {
    // LEGO MINDSTORMS EV3 Infrared Sensor
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_PROX          = (PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR << 8) | 0,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_SEEK          = (PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR << 8) | 1,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_REMOTE        = (PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR << 8) | 2,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_REM_A         = (PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR << 8) | 3,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_S_ALT         = (PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR << 8) | 4,
    PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_CAL           = (PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR << 8) | 5,
} pbio_iodev_mode_id_t;

/**
 * Data types used by I/O devices.
 */
typedef enum {
    /**
     * Signed 8-bit integer.
     */
    PBIO_IODEV_DATA_TYPE_INT8,
    /**
     * Little-endian, signed 16-bit integer.
     */
    PBIO_IODEV_DATA_TYPE_INT16,
    /**
     * Little-endian, signed 32-bit integer.
     */
    PBIO_IODEV_DATA_TYPE_INT32,
    /**
     * Little endian 32-bit floating point.
     */
    PBIO_IODEV_DATA_TYPE_FLOAT,
} pbio_iodev_data_type_t;

/**
 * The maximum number of modes a I/O device can have.
 */
#define PBIO_IODEV_MAX_NUM_MODES    16

/**
 * Max size of mode name (not including null terminator)
 */
#define PBIO_IODEV_MODE_NAME_SIZE   11

/**
 * Max number of data bytes for I/O data. This means that 32 8-bit values, 16
 * 16-bit values or 8 32-bit values are possible.
 */
#define PBIO_IODEV_MAX_DATA_SIZE    32

/**
 * Max size of units of measurements (not including null terminator)
 */
#define PBIO_IODEV_UOM_SIZE         4

/**
 * I/O device capability flags.
 */
typedef enum {
    PBIO_IODEV_FLAG_IS_MOTOR = 0x01,
} pbio_iodev_flag_t;

/**
 * Mapping flags that describe the input and output values of an I/O device.
 */
typedef enum {
    /** The value is a discrete value, e.g. a color index. */
    LPF2_MAPPING_FLAG_DISCRETE  = 1 << 2,
    /** The value is a relative value, e.g. a motor position. */
    LPF2_MAPPING_FLAG_RELATIVE  = 1 << 3,
    /** The value is an absolute value, e.g. a distance measurement. */
    LPF2_MAPPING_FLAG_ABSOLUTE  = 1 << 4,
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
     * The name of the mode
     */
    char name[PBIO_IODEV_MODE_NAME_SIZE + 1];
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
     * Capability flags.
     */
    pbio_iodev_flag_t flags;
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

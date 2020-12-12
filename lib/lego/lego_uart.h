// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

// LEGO UART Message Protocol (LUMP) for EV3 and Powered Up I/O devices
//
// This file contains bytecode definitions for interperting messages send to
// and from LEGO MINDSTORMS EV3 and Powered Up UART devices.
//
// References:
// - http://ev3.fantastic.computer/doxygen/UartProtocol.html
// - https://github.com/mindboards/ev3sources/blob/master/lms2012/d_uart/Linuxmod_AM1808/d_uart_mod.c
// - https://github.com/ev3dev/lego-linux-drivers/blob/ev3dev-buster/sensors/ev3_uart_sensor_ld.c
// - https://sourceforge.net/p/lejos/wiki/UART%20Sensor%20Protocol/
// - https://lego.github.io/lego-ble-wireless-protocol-docs/index.html


#ifndef _LEGO_UART_H_
#define _LEGO_UART_H_

#include <stdint.h>

// Header byte

/** Bit mask for ::lump_msg_type_t */
#define LUMP_MSG_TYPE_MASK 0xC0

/**
 * Message type.
 */
typedef enum {
    /**
     * System message type.
     *
     * These messages don't have a payload or a checksum, so only consist of the
     * single header byte.
     *
     * The ::LUMP_MSG_SIZE_MASK bits should be set to ::LUMP_MSG_SIZE_1.
     *
     * The ::LUMP_MSG_CMD_MASK bits must be one of ::lump_sys_t.
     */
    LUMP_MSG_TYPE_SYS = 0 << 6,

    /**
     * Command message type.
     *
     * The ::LUMP_MSG_SIZE_MASK bits must be set to the size of the
     * payload.
     *
     * The ::LUMP_MSG_CMD_MASK bits must be one of ::lump_cmd_t.
     */
    LUMP_MSG_TYPE_CMD = 1 << 6,

    /**
     * Info message type.
     *
     * The ::LUMP_MSG_SIZE_MASK bits must be set to the size of the
     * payload.
     *
     * The ::LUMP_MSG_CMD_MASK bits must be set to the mode index number.
     *
     * The header byte will be followed by a ::lump_info_t byte.
     */
    LUMP_MSG_TYPE_INFO = 2 << 6,

    /**
     * Data message type.
     *
     * The ::LUMP_MSG_SIZE_MASK bits must be set to the size of the
     * payload.
     *
     * The ::LUMP_MSG_CMD_MASK bits must be set to the mode index number.
     */
    LUMP_MSG_TYPE_DATA = 3 << 6,
} lump_msg_type_t;

/** Bit mask for ::lump_msg_size_t. */
#define LUMP_MSG_SIZE_MASK 0x38

/**
 * Macro to convert ::lump_msg_size_t to the size in bytes.
 *
 * @param [in]  s   ::lump_msg_size_t.
 * @return          The size in bytes.
 */
#define LUMP_MSG_SIZE(s) (1 << (((s) >> 3) & 0x7))

/**
 * Encoded message payload size.
 */
typedef enum {
    /** Payload is 1 byte. */
    LUMP_MSG_SIZE_1 = 0 << 3,

    /** Payload is 2 bytes. */
    LUMP_MSG_SIZE_2 = 1 << 3,

    /** Payload is 4 byte. */
    LUMP_MSG_SIZE_4 = 2 << 3,

    /** Payload is 8 byte. */
    LUMP_MSG_SIZE_8 = 3 << 3,

    /** Payload is 16 byte. */
    LUMP_MSG_SIZE_16 = 4 << 3,

    /** Payload is 32 byte. */
    LUMP_MSG_SIZE_32 = 5 << 3,
} lump_msg_size_t;

/**
 * The maximum message payload size (does not include header, info type or
 * checksum).
 */
#define LUMP_MAX_MSG_SIZE 32

/**
 * The message command or mode number mask.
 *
 * The meaning of the header value in this position depends on the
 * ::lump_msg_type_t of the header.
 */
#define LUMP_MSG_CMD_MASK 0x07

/**
 * The highest possible mode index number for EV3 I/O devices.
 *
 * On Powered Up devices, the actual highest index is ::LUMP_MAX_EXT_MODE, but
 * the extended mode value must be combined with the limited mode index number
 * to get the actual mode index so this value is still important when using
 * Powered Up devices. See ::LUMP_CMD_EXT_MODE and ::LUMP_INFO_MODE_PLUS_8.
 */
#define LUMP_MAX_MODE 7

/**
 * The highest possible mode index number for Powered Up I/O devices.
 */
#define LUMP_MAX_EXT_MODE 15

/**
 * System messages types.
 *
 * This value is encoded at ::LUMP_MSG_CMD_MASK when ::lump_msg_type_t is
 * ::LUMP_MSG_TYPE_SYS.
 */
typedef enum {
    /**
     * Syncronization message.
     *
     * This is the first message sent from a LEGO UART I/O device to the
     * controller.
     */
    LUMP_SYS_SYNC = 0x0,

    /**
     * Keep-alive message.
     *
     * The controller sends this to a LEGO UART I/O device periodically to keep
     * the I/O device from resetting.
     */
    LUMP_SYS_NACK = 0x2,

    /**
     * Acknowledgement message.
     *
     * This is sent by both the controller and the LEGO UART I/O device to
     * acknowledge successful completion of certian messages.
     */
    LUMP_SYS_ACK = 0x4,

    /** @cond UNUSED */
    LUMP_SYS_ESC = 0x6, // From EV3 source code - not used
    /** @endcond */
} lump_sys_t;

/**
 * Command types.
 *
 * This value is encoded at ::LUMP_MSG_CMD_MASK when ::lump_msg_type_t is
 * ::LUMP_MSG_TYPE_CMD.
 */
typedef enum {
    /**
     * Type id command.
     *
     * This message is sent from the I/O device to the controller.
     *
     * The payload is one byte that gives the unique type ID for an I/O device.
     * Known type IDs are listed in ::lump_type_id_t, but other type IDs are
     * possible.
     */
    LUMP_CMD_TYPE = 0x0,

    /**
     * Modes command.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The payload gives the last mode index number for a UART I/O device and
     * optionally the last mode index for the view modes. View modes are modes
     * that only return a single value. For example, on LEGO MINDSTORMS EV3,
     * view modes are the modes visible in the Port View and Data Log apps on
     * the brick.
     *
     * If the payload size is 1 byte, then that byte is the last mode index
     * number and also the the last view mode index number (limited to
     * ::LUMP_MAX_MODE).
     *
     * If the payload size is 2 bytes, then that byte is the last mode index
     * number and the second byte is the the last view mode index number
     * (limited to ::LUMP_MAX_MODE).
     *
     * If the payload size is 4 bytes (only possible on Powered Up), then the
     * first two bytes are as above and the last two bytes are as above except
     * they are not limited to ::LUMP_MAX_MODE.
     */
    LUMP_CMD_MODES = 0x1,

    /**
     * Speed command.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The payload has a size of 4 bytes and is a little-endian 32-bit unsigned
     * integer that gives the baud rate.
     *
     * On Powered Up devices, this message can also be sent from the controller
     * to the device before receiving ::LUMP_SYS_SYNC to test if the device
     * supports syncronization at a higher baud rate.
     *
     * It has also been observed that the payload can be 8 bytes where the last
     * 4 bytes are set to 1 (little-endian 32-bit unsigned integer) when sending
     * to ::LUMP_TYPE_ID_COLOR_DIST_SENSOR. This seems to have the affect of
     * making the light stop changing color during syncronization rather than
     * actually changing the baud rate.
     */
    LUMP_CMD_SPEED = 0x2,

    /**
     * Select mode command.
     *
     * This message is sent from the controller to the I/O device after
     * syncronization to select the mode.
     *
     * The payload has a size of one byte and specifies the mode index number
     * with a maximum value of ::LUMP_MAX_MODE.
     *
     * On Powered UP devices, a ::LUMP_CMD_EXT_MODE message must be sent first
     * to set the extended mode to 0 or 8, then the ::LUMP_CMD_SELECT message is
     * sent.
     */
    LUMP_CMD_SELECT = 0x3,

    /**
     * Write command.
     *
     * This message is sent in either directino after syncronization to select
     * the mode.
     *
     * The payload size and data depend on the ::lump_type_id_t of the I/O
     * device.
     *
     * Known uses of this command included resetting the angle on
     * ::LUMP_TYPE_ID_EV3_GYRO_SENSOR and selecting mode combinations on
     * Powered Up devices.
     */
    LUMP_CMD_WRITE = 0x4,

    /** @cond UNKNOWN */
    LUMP_CMD_UNK1 = 0x5, // Powered Up only
    /** @endcond */

    /**
     * Extended mode command.
     *
     * This message is sent from the controller to the I/O device after
     * syncronization to select the extended mode on Powered Up devices.
     *
     * Some Powered Up devices have more than 8 modes and therefore need extra
     * info when specifying modes.
     *
     * The payload is 1 byte and is either set to 0 or 8. This number is added
     * to the mode in the ::LUMP_CMD_SELECT command to get the actual mode.
     */
    LUMP_CMD_EXT_MODE = 0x6,

    /**
     * Version command.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization (Powered Up devices only).
     *
     * The payload is 8 bytes and gives the firmware and hardware version of
     * the device. The versions are encoded as little-endian 32-bit unsigned
     * integers in BCD format (each 4 bits is a digit 0-9). The major version
     * is the first 4 bits (one digit). The minor version is next four bits
     * (one digit). The bug fix version is the second byte (2 digits) and the
     * build number is the last two bytes (4 digits).
     */
    LUMP_CMD_VERSION = 0x7,
} lump_cmd_t;

/**
 * Mode information message type.
 *
 * This byte is sent immediately after the message header when ::lump_msg_type_t
 * is ::LUMP_MSG_TYPE_INFO. It is not considered part of the message payload
 * size.
 */
typedef enum {
    /**
     * Mode name message.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The payload size is variable and gives the name of the mode in ASCII
     * characters. The length of the mode name is at most ::LUMP_MAX_NAME_SIZE.
     * This message is always the first ::LUMP_MSG_TYPE_INFO message to be sent
     * for each mode.
     *
     * On Powered up devices, the payload may also include additional
     * ::lump_mode_flags_t bytes. In this case, the name will be limited
     * to the first 6 bytes (includes zero-termination character, so name
     * itself is limited to ::LUMP_MAX_SHORT_NAME_SIZE characters). The next
     * 6 bytes contain the ::lump_mode_flags_t data.
     */
    LUMP_INFO_NAME = 0x00,

    /**
     * Mode raw data scaling message.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The payload is 8 bytes and contains the minimum and maximum raw data
     * scaling values for the mode in little-endian 32-bit floating point
     * format.
     */
    LUMP_INFO_RAW = 0x01,

    /**
     * Mode percentage data scaling message.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The payload is 8 bytes and contains the minimum and maximum percentage
     * data scaling values for the mode in little-endian 32-bit floating point
     * format.
     */
    LUMP_INFO_PCT = 0x02,

    /**
     * Mode scaled data scaling message.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The payload is 8 bytes and contains the minimum and maximum scaled
     * data scaling values for the mode in little-endian 32-bit floating point
     * format.
     *
     * Scaled data has the units of measurement given by ::LUMP_INFO_UNITS.
     */
    LUMP_INFO_SI = 0x03,

    /**
     * Mode unit of measurement message.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The payload is up to ::LUMP_MAX_UOM_SIZE bytes and gives the units of
     * measurement of the mode.
     */
    LUMP_INFO_UNITS         = 0x04,

    LUMP_INFO_MAPPING       = 0x05,    // Powered Up only
    LUMP_INFO_MODE_COMBOS   = 0x06,    // Powered Up only
    LUMP_INFO_UNK7          = 0x07,    // Powered Up only
    LUMP_INFO_UNK8          = 0x08,    // Powered Up only
    LUMP_INFO_UNK9          = 0x09,    // Powered Up only
    LUMP_INFO_UNK10         = 0x0A,    // Powered Up only
    LUMP_INFO_UNK11         = 0x0B,    // Powered Up only
    LUMP_INFO_UNK12         = 0x0C,    // Powered Up only

    /**
     * Mode unit of measurement message.
     *
     * This is a flag rather than a message type and can be combined with other
     * ::lump_info_t (using bitwise or). It indicates that the current mode is 8
     * plus the mode given in the ::LUMP_MSG_CMD_MASK of the header. This is
     * required for mode index numbers 8 and higher since :LUMP_MSG_CMD_MASK is
     * limited to ::LUMP_MAX_MODE. (Only seen on Powered Up devices.)
     */
    LUMP_INFO_MODE_PLUS_8 = 0x20,

    /**
     * Mode format message.
     *
     * This message is sent from the I/O device to the controller during
     * syncronization.
     *
     * The messages describes the format of the data that will be sent in
     * ::LUMP_MSG_TYPE_DATA messages after syncronization is complete. This
     * message is always the last ::LUMP_MSG_TYPE_INFO message to be sent for
     * each mode.
     *
     * The payload is 4 bytes. The first byte gives the number of values in the
     * data. The second byte is one of ::lump_data_type_t. The 3rd byte gives
     * the number of digits needed to display the value. The 4th byte gives
     * the number of decimal points to use (e.g. if decimals is 1, the data
     * values need to be divied by 10 to get the actual value).
     */
    LUMP_INFO_FORMAT        = 0x80,
} lump_info_t;

/**
 * Type IDs.
 *
 * Sent in ::LUMP_CMD_TYPE messages.
 *
 * Each type of I/O device has a unique type identifier. The following are the
 * known type IDs, but other IDs are allowed.
 */
typedef enum {
    /** MINDSTORMS EV3 Color Sensor */
    LUMP_TYPE_ID_EV3_COLOR_SENSOR         = 29,

    /** MINDSTORMS EV3 Ultrasonic Sensor */
    LUMP_TYPE_ID_EV3_ULTRASONIC_SENSOR    = 30,

    /** MINDSTORMS EV3 Gyro Sensor */
    LUMP_TYPE_ID_EV3_GYRO_SENSOR          = 32,

    /** MINDSTORMS EV3 Infrared Sensor */
    LUMP_TYPE_ID_EV3_IR_SENSOR            = 33,

    /** WeDo 2.0 Tilt Sensor */
    LUMP_TYPE_ID_WEDO2_TILT_SENSOR        = 34,

    /** WeDo 2.0 Motion Sensor */
    LUMP_TYPE_ID_WEDO2_MOTION_SENSOR      = 35,

    /** BOOST Color and Distance Sensor */
    LUMP_TYPE_ID_COLOR_DIST_SENSOR        = 37,

    /** BOOST Interactive Motor */
    LUMP_TYPE_ID_INTERACTIVE_MOTOR        = 38,

    /** Technic Large Motor */
    LUMP_TYPE_ID_TECHNIC_L_MOTOR          = 46,

    /** Technic XL Motor */
    LUMP_TYPE_ID_TECHNIC_XL_MOTOR         = 47,

    /** SPIKE Medium Motor */
    LUMP_TYPE_ID_SPIKE_M_MOTOR            = 48,

    /** SPIKE Large Motor */
    LUMP_TYPE_ID_SPIKE_L_MOTOR            = 49,

    /** SPIKE Color Sensor */
    LUMP_TYPE_ID_SPIKE_COLOR_SENSOR       = 61,

    /** SPIKE Ultrasonic Sensor */
    LUMP_TYPE_ID_SPIKE_ULTRASONIC_SENSOR  = 62,

    /** SPIKE Prime Force Sensor */
    LUMP_TYPE_ID_SPIKE_FORCE_SENSOR       = 63,

    /** Technic Medium Angular Motor */
    LUMP_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR  = 75,

    /** Technic Large Angular Motor */
    LUMP_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR  = 76,

} lump_type_id_t;

/**
 * Maximum size of mode name (excluding zero-termination character) sent in
 * ::LUMP_INFO_NAME message.
 */
#define LUMP_MAX_NAME_SIZE 11

/**
 * Maximum size of mode name (excluding zero-termination character) sent in
 * ::LUMP_INFO_NAME message when ::lump_mode_flags_t is also included in the
 * same message.
 */
#define LUMP_MAX_SHORT_NAME_SIZE 5

/**
 * Maximum size of units of measurements (excluding zero-termination character)
 * sent in ::LUMP_INFO_UNITS message.
 */
#define LUMP_MAX_UOM_SIZE 4

// There is currently no documentation from LEGO about mode flags, so these are
// guesses as to the meanings.

/**
 * Data type.
 *
 * Sent in ::LUMP_INFO_FORMAT messges.
 */
typedef enum {
    /** 8-bit signed integer. */
    LUMP_DATA_TYPE_DATA8 = 0x00,

    /** little-endian 16-bit signed integer. */
    LUMP_DATA_TYPE_DATA16 = 0x01,

    /** little-endian 32-bit signed integer. */
    LUMP_DATA_TYPE_DATA32 = 0x02,

    /** little-endian 32-bit floating point. */
    LUMP_DATA_TYPE_DATAF = 0x03,
} lump_data_type_t;

/**
 * Mode flags group 0.
 */
typedef enum {
    /** No flags are set. */
    LUMP_MODE_FLAGS0_NONE = 0,

    /** This mode is a `SPEED` mode. */
    LUMP_MODE_FLAGS0_MOTOR_SPEED = 1 << 0,

    /** This mode is a `ABS` mode. */
    LUMP_MODE_FLAGS0_MOTOR_ABS_POS = 1 << 1,

    /** This mode is a `POS` mode. */
    LUMP_MODE_FLAGS0_MOTOR_REL_POS = 1 << 2,

    /** This mode is a `POWER` mode. */
    LUMP_MODE_FLAGS0_MOTOR_POWER = 1 << 4,

    /** This mode is a motor mode. */
    LUMP_MODE_FLAGS0_MOTOR = 1 << 5,

    /** This sensor mode requires battery voltage, not just logic voltage. */
    LUMP_MODE_FLAGS0_REQUIRES_POWER = 1 << 6,
} lump_mode_flags0_t;

/**
 * Mode flags group 1.
 */
typedef enum {
    /** No flags are set. */
    LUMP_MODE_FLAGS1_NONE = 0,

    /** This mode is a `CALIB` mode. */
    LUMP_MODE_FLAGS1_CALIB = 1 << 6,
} lump_mode_flags1_t;

/**
 * Mode flags group 4.
 */
typedef enum {
    /** No flags are set. */
    LUMP_MODE_FLAGS4_NONE = 0,

    /** This mode uses power out on pins 1 and 2. */
    LUMP_MODE_FLAGS4_USES_HBRIDGE = 1 << 0,

    LUMP_MODE_FLAGS4_UNKNOWN_BIT2 = 1 << 2,
} lump_mode_flags4_t;

/**
 * Mode flags group 4.
 */
typedef enum {
    /** No flags are set. */
    LUMP_MODE_FLAGS5_NONE = 0,

    LUMP_MODE_FLAGS5_UNKNOWN_BIT1 = 1 << 1,
    LUMP_MODE_FLAGS5_UNKNOWN_BIT2 = 1 << 2,
    LUMP_MODE_FLAGS5_UNKNOWN_BIT7 = 1 << 7,
} lump_mode_flags5_t;

/**
 * Mode flags.
 *
 * These flags are sent from newer UART I/O devices (starting with
 * Technic motors) along with the ::LUMP_INFO_NAME message.
 */
typedef struct {
    /**
     * ::lump_mode_flags0_t
     */
    uint8_t flags0;

    /**
     * ::lump_mode_flags1_t
     */
    uint8_t flags1;

    uint8_t flags2; // always 0?

    uint8_t flags3; // always 0?

    /**
     * ::lump_mode_flags4_t
     */
    uint8_t flags4;

    /**
     * ::lump_mode_flags5_t
     */
    uint8_t flags5;
} lump_mode_flags_t;

#endif // _LEGO_UART_H_

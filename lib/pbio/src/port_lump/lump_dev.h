// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Shared internals of the LEGO UART device implementation.
//
// protocol.c implements the LUMP protocol and knows as little as possible
// about individual devices. sensors.c, telemetry.c and ev3_color.c implement
// what the data of each known device means.

#ifndef _PBIO_PORT_LUMP_LUMP_DEV_H_
#define _PBIO_PORT_LUMP_LUMP_DEV_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/port_lump.h>

#include <lego/device.h>
#include <lego/lump.h>

#define PBIO_PORT_LUMP_DEBUG (0)

#if PBIO_PORT_LUMP_DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbio/debug.h>
#define debug_pr pbio_debug
#else
#define debug_pr(...)
#endif

/**
 * Indicates the current state of the UART device.
 */
typedef enum {
    /** Something bad happened. */
    PBIO_PORT_LUMP_STATUS_ERR,
    /** Waiting for data that looks like LEGO UART protocol. */
    PBIO_PORT_LUMP_STATUS_SYNCING,
    /** Reading device info before changing baud rate. */
    PBIO_PORT_LUMP_STATUS_INFO,
    /** ACK received, delay changing baud rate. */
    PBIO_PORT_LUMP_STATUS_ACK,
    /** Ready to send commands and receive data. */
    PBIO_PORT_LUMP_STATUS_DATA,
} pbio_port_lump_status_t;

typedef struct {
    /** The mode to be set. */
    uint8_t desired_mode;
    /** Whether a mode change was requested (set low when handled). */
    bool requested;
    /** Time of switch completion (if info.mode == desired_mode) or time of switch request (if info.mode != desired_mode). */
    uint32_t time;
} pbio_port_lump_mode_switch_t;

typedef struct {
    /** The data to be set. */
    uint8_t bin_data[LUMP_MAX_MSG_SIZE]  __attribute__((aligned(4)));
    /** The size of the data to be set, also acts as set request flag. */
    uint8_t size;
    /** The mode at which to set data */
    uint8_t desired_mode;
    /** Time of the data set request (if size != 0) or time of completing transmission (if size == 0). */
    uint32_t time;
} pbio_port_lump_data_set_t;

#if PBIO_CONFIG_PORT_LUMP_EV3

typedef enum {
    EV3_COLOR_CALIBRATION_STATE_WAIT_FOR_USER,
    EV3_COLOR_CALIBRATION_STATE_RGB_BALANCING,
    EV3_COLOR_CALIBRATION_STATE_DONE_SUCCESS,
    EV3_COLOR_CALIBRATION_STATE_DONE_FAILURE,
} pbio_port_lump_ev3_color_calibration_state_t;

typedef struct {
    /** White balance gains, loaded from storage when this sensor syncs up. */
    float gain[3];
    /** Protothread state of the calibration routine. Zero when not running. */
    pbio_os_state_t thread;
    pbio_os_timer_t timer;
    pbio_port_lump_ev3_color_calibration_state_t calibration_state;
    /** Running sums of the samples taken in the current calibration step. */
    float sum[3];
    /** Number of samples taken in the current calibration step. */
    uint8_t count;
    /** Spread of COL-REFLECT during its calibration step, to detect movement. */
    int16_t reflect_min;
    int16_t reflect_max;
    /** Mean COL-REFLECT and mean raw red-on/red-off difference. */
    float reflect;
    float delta;
} pbio_port_lump_ev3_color_state_t;

#endif // PBIO_CONFIG_PORT_LUMP_EV3

/**
 * Device specific state, reset when a device syncs up.
 *
 * Only one device can be attached to a port at a time, so these never overlap
 * in use. Which member is valid follows from the device type and its mode.
 */
typedef union {
    /** Angle of motors that report rotation. */
    pbio_angle_t angle;
    /** Force sensor calibration constants, read from the sensor. */
    struct {
        int16_t offset;
        int16_t released;
        int16_t end;
    } force;
    /** Number of samples received from the color light matrix, up to 2. */
    uint8_t light_matrix_samples;
    #if PBIO_CONFIG_PORT_LUMP_EV3
    pbio_port_lump_ev3_color_state_t ev3_color;
    #endif
} pbio_port_lump_device_state_t;

/** LUMP state for one port. */
struct _pbio_port_lump_dev_t {
    /** Child protothread of the main protothread used for reading data */
    pbio_os_state_t read_pt;
    /** Child protothread of the main protothread used for writing data */
    pbio_os_state_t write_pt;
    /** Buffer to hold messages received from the device. */
    uint8_t *rx_msg;
    /** Buffer to hold messages transmitted to the device. */
    uint8_t *tx_msg;
    /** Data set buffer and status. */
    pbio_port_lump_data_set_t *data_set;
    /**
     * Most recent binary data read from the device. How to interpret this data
     * is determined by the ::pbio_port_lump_mode_info_t info associated with the current
     * *mode* of the device. For example, it could be an array of int32_t and/or
     * the values could be foreign-endian.
     */
    uint8_t *bin_data;
    /** Index of this device, used to find its settings in storage. */
    uint8_t index;
    /**< The type identifier of the device. */
    lego_device_type_id_t type_id;
    /** The current mode of the device */
    uint8_t mode;
    /**< The capabilities and requirements of the device. */
    uint8_t capabilities;
    /** The current device connection state. */
    pbio_port_lump_status_t status;
    /** Mode switch status. */
    pbio_port_lump_mode_switch_t mode_switch;
    /** Extra mode adder for Powered Up devices (for modes > LUMP_MAX_MODE). */
    uint8_t ext_mode;
    /** New baud rate that will be set with ev3_uart_change_bitrate. */
    uint32_t new_baud_rate;
    /** Size of the current message being transmitted. */
    uint32_t tx_msg_size;
    /** Size of the current message being received. */
    uint32_t rx_msg_size;
    /** Total number of errors that have occurred. Re-used in different stages of synchronization and data reading. */
    uint32_t err_count;
    /** Flag that indicates that good DATA lump_dev->msg has been received since last watchdog timeout. */
    bool data_rec;
    /** State of the particular device that is attached. */
    pbio_port_lump_device_state_t device;
    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
    /** Mode value used to keep track of mode in INFO messages while syncing. */
    uint8_t new_mode;
    /** Flags indicating what information has already been read from the data. */
    uint32_t info_flags;
    /**< The number of modes */
    uint8_t num_modes;
    /**< Information about the current mode. */
    pbio_port_lump_mode_info_t mode_info[(LUMP_MAX_EXT_MODE + 1)];
    #endif // PBIO_CONFIG_PORT_LUMP_MODE_INFO
};

// Implemented by protocol.c.

void pbio_port_lump_request_mode(pbio_port_lump_dev_t *lump_dev, uint8_t mode);

// Implemented by sensors.c.

bool pbio_port_lump_is_relative_motor(pbio_port_lump_dev_t *lump_dev);

bool pbio_port_lump_is_absolute_motor(pbio_port_lump_dev_t *lump_dev);

void pbio_port_lump_device_init(pbio_port_lump_dev_t *lump_dev);

void pbio_port_lump_device_handle_data(pbio_port_lump_dev_t *lump_dev);

uint32_t pbio_port_lump_device_stale_data_delay(pbio_port_lump_dev_t *lump_dev);

uint32_t pbio_port_lump_device_data_set_delay(pbio_port_lump_dev_t *lump_dev);

#if PBIO_CONFIG_PORT_LUMP_EV3

// Implemented by ev3_color.c.

void pbio_port_lump_ev3_color_load_calibration(pbio_port_lump_dev_t *lump_dev);

bool pbio_port_lump_ev3_color_calibration_busy(pbio_port_lump_dev_t *lump_dev);

void pbio_port_lump_ev3_color_calibration_start(pbio_port_lump_dev_t *lump_dev);

void pbio_port_lump_ev3_color_handle_data(pbio_port_lump_dev_t *lump_dev);

void pbio_port_lump_ev3_color_get_rgb(pbio_port_lump_dev_t *lump_dev, pbio_color_rgb_t *rgb);

#endif // PBIO_CONFIG_PORT_LUMP_EV3

#endif // _PBIO_PORT_LUMP_LUMP_DEV_H_

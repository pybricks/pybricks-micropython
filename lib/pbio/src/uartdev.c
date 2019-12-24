// SPDX-License-Identifier: MIT OR GPL-2.0-only
// Copyright (c) 2018 David Lechner

/*
 * Based on:
 * LEGO MINDSTORMS EV3 UART Sensor tty line discipline
 *
 * Copyright (C) 2014-2016,2018-2019 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Relicensed under MIT license by author for Pybricks I/O library.
 * This file may be redistributed under either or both licenses.
 */

#include <pbio/config.h>

#if PBIO_CONFIG_UARTDEV

#define DEBUG 0
#if DEBUG
#include <inttypes.h>
#define debug_pr(fmt, ...)   printf((fmt), __VA_ARGS__)
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>

#include "pbdrv/config.h"
#include "pbdrv/ioport.h"
#include "pbdrv/uart.h"
#include "pbio/error.h"
#include "pbio/event.h"
#include "pbio/iodev.h"
#include "pbio/port.h"
#include "pbio/uartdev.h"
#include "pbio/util.h"
#include "../drv/counter/counter.h"

#define EV3_UART_MAX_MESSAGE_SIZE   (PBIO_IODEV_MAX_DATA_SIZE + 2)

#define EV3_UART_MSG_TYPE_MASK      0xC0
#define EV3_UART_MSG_SIZE_MASK      0x38
#define EV3_UART_CMD_SIZE(byte)     (1 << (((byte) >> 3) & 0x7))
#define EV3_UART_MSG_CMD_MASK       0x07
#define EV3_UART_MAX_DATA_ERR       6

#define EV3_UART_TYPE_MIN           29      // EV3 color sensor
#define EV3_UART_TYPE_MAX           101
#define EV3_UART_SPEED_MIN          2400
#define EV3_UART_SPEED_MAX          460800  // in practice 115200 is max
#define EV3_UART_MODE_MAX           7       // Powered Up devices can have max of 15 modes
                                            // by using EV3_UART_INFO_MODE_PLUS_8

#define EV3_UART_DATA_KEEP_ALIVE_TIMEOUT    100 /* msec */
#define EV3_UART_IO_TIMEOUT                 250 /* msec */

enum ev3_uart_msg_type {
    EV3_UART_MSG_TYPE_SYS   = 0x00,
    EV3_UART_MSG_TYPE_CMD   = 0x40,
    EV3_UART_MSG_TYPE_INFO  = 0x80,
    EV3_UART_MSG_TYPE_DATA  = 0xC0,
};

enum ev3_uart_msg_size {
    EV3_UART_MSG_SIZE_1     = 0x0 << 3,
    EV3_UART_MSG_SIZE_2     = 0x1 << 3,
    EV3_UART_MSG_SIZE_4     = 0x2 << 3,
    EV3_UART_MSG_SIZE_8     = 0x3 << 3,
    EV3_UART_MSG_SIZE_16    = 0x4 << 3,
    EV3_UART_MSG_SIZE_32    = 0x5 << 3,
};

enum ev3_uart_sys {
    EV3_UART_SYS_SYNC       = 0x0,
    EV3_UART_SYS_NACK       = 0x2,
    EV3_UART_SYS_ACK        = 0x4,
    EV3_UART_SYS_ESC        = 0x6,  // From EV3 source code - not used
};

enum ev3_uart_cmd {
    EV3_UART_CMD_TYPE       = 0x0,
    EV3_UART_CMD_MODES      = 0x1,
    EV3_UART_CMD_SPEED      = 0x2,
    EV3_UART_CMD_SELECT     = 0x3,
    EV3_UART_CMD_WRITE      = 0x4,
    EV3_UART_CMD_UNK1       = 0x5,    // Powered Up only
    EV3_UART_CMD_EXT_MODE   = 0x6,    // Powered Up only
    EV3_UART_CMD_VERSION    = 0x7,    // Powered Up only
};

enum ev3_uart_info {
    EV3_UART_INFO_NAME          = 0x00,
    EV3_UART_INFO_RAW           = 0x01,
    EV3_UART_INFO_PCT           = 0x02,
    EV3_UART_INFO_SI            = 0x03,
    EV3_UART_INFO_UNITS         = 0x04,
    EV3_UART_INFO_MAPPING       = 0x05,    // Powered Up only
    EV3_UART_INFO_MODE_COMBOS   = 0x06,    // Powered Up only
    EV3_UART_INFO_MOTOR_BIAS    = 0x07,    // Powered Up only
    EV3_UART_INFO_CAPABILITY    = 0x08,    // Powered Up only
    EV3_UART_INFO_MODE_PLUS_8   = 0x20,    // Powered Up only
    EV3_UART_INFO_FORMAT        = 0x80,
};

enum ev3_uart_info_bit {
    EV3_UART_INFO_BIT_CMD_TYPE,
    EV3_UART_INFO_BIT_CMD_MODES,
    EV3_UART_INFO_BIT_CMD_SPEED,
    EV3_UART_INFO_BIT_CMD_VERSION,
    EV3_UART_INFO_BIT_INFO_NAME,
    EV3_UART_INFO_BIT_INFO_RAW,
    EV3_UART_INFO_BIT_INFO_PCT,
    EV3_UART_INFO_BIT_INFO_SI,
    EV3_UART_INFO_BIT_INFO_UNITS,
    EV3_UART_INFO_BIT_INFO_MAPPING,
    EV3_UART_INFO_BIT_INFO_MODE_COMBOS,
    EV3_UART_INFO_BIT_INFO_MOTOR_BIAS,
    EV3_UART_INFO_BIT_INFO_CAPABILITY,
    EV3_UART_INFO_BIT_INFO_FORMAT,
};

enum ev3_uart_info_flags {
    EV3_UART_INFO_FLAG_CMD_TYPE                 = 1 << EV3_UART_INFO_BIT_CMD_TYPE,
    EV3_UART_INFO_FLAG_CMD_MODES                = 1 << EV3_UART_INFO_BIT_CMD_MODES,
    EV3_UART_INFO_FLAG_CMD_SPEED                = 1 << EV3_UART_INFO_BIT_CMD_SPEED,
    EV3_UART_INFO_FLAG_CMD_VERSION              = 1 << EV3_UART_INFO_BIT_CMD_VERSION,
    EV3_UART_INFO_FLAG_INFO_NAME                = 1 << EV3_UART_INFO_BIT_INFO_NAME,
    EV3_UART_INFO_FLAG_INFO_RAW                 = 1 << EV3_UART_INFO_BIT_INFO_RAW,
    EV3_UART_INFO_FLAG_INFO_PCT                 = 1 << EV3_UART_INFO_BIT_INFO_PCT,
    EV3_UART_INFO_FLAG_INFO_SI                  = 1 << EV3_UART_INFO_BIT_INFO_SI,
    EV3_UART_INFO_FLAG_INFO_UNITS               = 1 << EV3_UART_INFO_BIT_INFO_UNITS,
    EV3_UART_INFO_FLAG_INFO_MAPPING             = 1 << EV3_UART_INFO_BIT_INFO_MAPPING,
    EV3_UART_INFO_FLAG_INFO_MODE_COMBOS         = 1 << EV3_UART_INFO_BIT_INFO_MODE_COMBOS,
    EV3_UART_INFO_FLAG_INFO_MOTOR_BIAS          = 1 << EV3_UART_INFO_BIT_INFO_MOTOR_BIAS,
    EV3_UART_INFO_FLAG_INFO_MOTOR_CAPABILITY    = 1 << EV3_UART_INFO_BIT_INFO_CAPABILITY,
    EV3_UART_INFO_FLAG_INFO_FORMAT              = 1 << EV3_UART_INFO_BIT_INFO_FORMAT,

    EV3_UART_INFO_FLAG_ALL_INFO     = EV3_UART_INFO_FLAG_INFO_NAME
                                    | EV3_UART_INFO_FLAG_INFO_RAW
                                    | EV3_UART_INFO_FLAG_INFO_PCT
                                    | EV3_UART_INFO_FLAG_INFO_SI
                                    | EV3_UART_INFO_FLAG_INFO_UNITS
                                    | EV3_UART_INFO_FLAG_INFO_MAPPING
                                    | EV3_UART_INFO_FLAG_INFO_MODE_COMBOS
                                    | EV3_UART_INFO_FLAG_INFO_MOTOR_BIAS
                                    | EV3_UART_INFO_FLAG_INFO_MOTOR_CAPABILITY
                                    | EV3_UART_INFO_FLAG_INFO_FORMAT,
    EV3_UART_INFO_FLAG_REQUIRED     = EV3_UART_INFO_FLAG_CMD_TYPE
                                    | EV3_UART_INFO_FLAG_CMD_MODES
                                    | EV3_UART_INFO_FLAG_INFO_NAME
                                    | EV3_UART_INFO_FLAG_INFO_FORMAT,
};

enum ev3_uart_data_type {
    EV3_UART_DATA_8		= 0x00,
    EV3_UART_DATA_16	= 0x01,
    EV3_UART_DATA_32	= 0x02,
    EV3_UART_DATA_FLOAT	= 0x03,
};

/**
 * Indicates the current state of the UART device.
 */
typedef enum {
    PBIO_UARTDEV_STATUS_ERR,        /**< Something bad happended */
    PBIO_UARTDEV_STATUS_SYNCING,    /**< Waiting for data that looks like LEGO UART protocol */
    PBIO_UARTDEV_STATUS_INFO,       /**< Reading device info before changing baud rate */
    PBIO_UARTDEV_STATUS_ACK,        /**< ACK received, delay changing baud rate */
    PBIO_UARTDEV_STATUS_DATA,       /**< Ready to send commands and receive data */
} pbio_uartdev_status_t;

/**
 * struct ev3_uart_port_data - Data for EV3/LPF2 UART Sensor communication
 * @iodev: The I/O device state information struct
 * @counter_dev: A counter device to provide access to tacho counts
 * @pt: Protothread for main communication protocol
 * @pt_data: Protothread for receiving sensor data
 * @timer: Timer for sending keepalive messages and other delays.
 * @uart: Pointer to the UART device to use for communications
 * @info: The I/O device information struct for the connected device
 * @status: The current device connection state
 * @type_id: The type ID received
 * @requested_mode: Mode that was requested by user. Used to restore previous
 * 	mode in case of a reconnect.
 * @new_mode: The mode requested by set_mode. Also used to keep track of mode
 *  in INFO messages while syncing.
 * @new_baud_rate: New baud rate that will be set with ev3_uart_change_bitrate
 * @info_flags: Flags indicating what information has already been read
 * 	from the data.
 * @tacho_count: The tacho count received from an LPF2 motor
 * @tx_msg: Buffer to hold messages transmitted to the device
 * @rx_msg: Buffer to hold messages received from the device
 * @rx_msg_size: Size of the current message being received
 * @ext_mode: Extra mode adder for Powered Up devices (for modes > EV3_UART_MODE_MAX)
 * @write_cmd_size: The size parameter received from a WRITE command
 * @tacho_rate: The tacho rate received from an LPF2 motor
 * @last_err: data->msg to be printed in case of an error.
 * @err_count: Total number of errors that have occurred
 * @num_data_err: Number of bad reads when receiving DATA data->msgs.
 * @data_rec: Flag that indicates that good DATA data->msg has been received
 * 	since last watchdog timeout.
 * @tx_busy: mutex that protects tx_msg
 */
typedef struct {
    pbio_iodev_t iodev;
    pbdrv_counter_dev_t counter_dev;
    struct pt pt;
    struct pt data_pt;
    struct etimer timer;
    pbdrv_uart_dev_t *uart;
    pbio_iodev_info_t *info;
    pbio_uartdev_status_t status;
    pbio_iodev_type_id_t type_id;
    uint8_t requested_mode;
    uint8_t new_mode;
    uint32_t new_baud_rate;
    uint32_t info_flags;
    int32_t tacho_count;
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t rx_msg_size;
    uint8_t ext_mode;
    uint8_t write_cmd_size;
    int8_t tacho_rate;
    DBG_ERR(const char *last_err);
    uint32_t err_count;
    uint32_t num_data_err;
    bool data_rec;
    bool tx_busy;
} uartdev_port_data_t;

enum {
    BUF_TX_MSG,
    BUF_RX_MSG,
    NUM_BUF
};

PROCESS(pbio_uartdev_process, "UART device");

static struct {
    pbio_iodev_info_t info;
    pbio_iodev_mode_t modes[PBIO_IODEV_MAX_NUM_MODES];
} infos[PBIO_CONFIG_UARTDEV_NUM_DEV];

static uint8_t bufs[PBIO_CONFIG_UARTDEV_NUM_DEV][NUM_BUF][EV3_UART_MAX_MESSAGE_SIZE];

static uartdev_port_data_t dev_data[PBIO_CONFIG_UARTDEV_NUM_DEV];

static const pbio_iodev_mode_t ev3_uart_default_mode_info = {
    .raw_max    = 1023,
    .pct_max    = 100,
    .si_max     = 1,
    .digits     = 4,
};

#define PBIO_PT_WAIT_READY(pt, expr) PT_WAIT_UNTIL((pt), (expr) != PBIO_ERROR_AGAIN)

pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev) {
    if (id >= PBIO_CONFIG_UARTDEV_NUM_DEV) {
        return PBIO_ERROR_NO_DEV;
    }

    if (!dev_data[id].iodev.info) {
        // device has not been initalized yet
        return PBIO_ERROR_AGAIN;
    }

    *iodev = &dev_data[id].iodev;

    return PBIO_SUCCESS;
}

static inline uint32_t uint32_le(uint8_t *bytes) {
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

static inline float float_le(uint8_t *bytes) {
    union {
        float f;
        uint32_t u;
    } result;

    result.u = uint32_le(bytes);

    return result.f;
}

static inline bool test_and_set_bit(uint8_t bit, uint32_t *flags) {
    bool result = *flags & (1 << bit);
    *flags |= (1 << bit);
    return result;
}

static uint8_t ev3_uart_get_msg_size(uint8_t header) {
    uint8_t size;

    if ((header & EV3_UART_MSG_TYPE_MASK) == EV3_UART_MSG_TYPE_SYS) {
        return 1;
    }

    size = EV3_UART_CMD_SIZE(header);
    size += 2; /* header and checksum */
    if ((header & EV3_UART_MSG_TYPE_MASK) == EV3_UART_MSG_TYPE_INFO)
        size++; /* extra command byte */

    return size;
}

static void pbio_uartdev_parse_msg(uartdev_port_data_t *data) {
    uint32_t speed;
    uint8_t msg_type, cmd, msg_size, mode, cmd2;

    msg_type = data->rx_msg[0] & EV3_UART_MSG_TYPE_MASK;
    cmd = data->rx_msg[0] & EV3_UART_MSG_CMD_MASK;
    msg_size = ev3_uart_get_msg_size(data->rx_msg[0]);
    mode = cmd;
    cmd2 = data->rx_msg[1];

    // The original EV3 spec only allowed for up to 8 modes (3-bit number).
    // The Powered UP spec extents this by adding an extra flag to INFO commands.
    // Not sure that EV3_UART_INFO_MODE_PLUS_8 is used in practice, but rather
    // an extra (separate) EV3_UART_CMD_EXT_MODE message seems to be used instead
    if (msg_type == EV3_UART_MSG_TYPE_INFO && (cmd2 & EV3_UART_INFO_MODE_PLUS_8)) {
        mode += 8;
        cmd2 &= ~EV3_UART_INFO_MODE_PLUS_8;
    }
    else {
        mode += data->ext_mode;
    }

    if (msg_size > 1) {
        uint8_t checksum = 0xFF;
        for (int i = 0; i < msg_size - 1; i++) {
            checksum ^= data->rx_msg[i];
        }
        if (checksum != data->rx_msg[msg_size - 1]) {
            DBG_ERR(data->last_err = "Bad checksum");
            // if INFO messages are done and we are now receiving data, it is
            // OK to occasionally have a bad checksum
            if (data->status == PBIO_UARTDEV_STATUS_DATA) {

                // The LEGO EV3 color sensor sends bad checksums
                // for RGB-RAW data (mode 4). The check here could be
                // improved if someone can find a pattern.
                if (data->type_id != PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR
                    || data->rx_msg[0] != (EV3_UART_MSG_TYPE_DATA | EV3_UART_MSG_SIZE_8 | 4)) {
                    return;
                }
            } else {
                goto err;
            }
        }
    }

    switch (msg_type) {
    case EV3_UART_MSG_TYPE_SYS:
        switch(cmd) {
        case EV3_UART_SYS_SYNC:
            /* IR sensor (type 33) sends checksum after SYNC */
            if (msg_size > 1 && (cmd ^ cmd2) == 0xFF) {
                msg_size++;
            }
            break;
        case EV3_UART_SYS_ACK:
            if (!data->info->num_modes) {
                DBG_ERR(data->last_err = "Received ACK before all mode INFO");
                goto err;
            }
            if ((data->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                DBG_ERR(data->last_err = "Did not receive all required INFO");
                goto err;
            }

            data->status = PBIO_UARTDEV_STATUS_ACK;
            data->iodev.mode = data->new_mode;

            return;
        }
        break;
    case EV3_UART_MSG_TYPE_CMD:
        switch (cmd) {
        case EV3_UART_CMD_MODES:
            if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_MODES, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate modes INFO");
                goto err;
            }
            if (cmd2 > EV3_UART_MODE_MAX) {
                DBG_ERR(data->last_err = "Number of modes is out of range");
                goto err;
            }
            data->info->num_modes = cmd2 + 1;
            if (msg_size > 5) {
                // Powered Up devices can have an extended mode message that
                // includes modes > EV3_UART_MODE_MAX
                data->info->num_modes = data->rx_msg[3] + 1;
                data->info->num_view_modes = data->rx_msg[4] + 1;
            }
            else if (msg_size > 3) {
                data->info->num_view_modes = data->rx_msg[2] + 1;
            }
            else {
                data->info->num_view_modes = data->info->num_modes;
            }

            debug_pr("num_modes: %d\n", data->info->num_modes);
            debug_pr("num_view_modes: %d\n", data->info->num_view_modes);

            break;
        case EV3_UART_CMD_SPEED:
            if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_SPEED, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate speed INFO");
                goto err;
            }
            speed = uint32_le(data->rx_msg + 1);
            if (speed < EV3_UART_SPEED_MIN || speed > EV3_UART_SPEED_MAX) {
                DBG_ERR(data->last_err = "Speed is out of range");
                goto err;
            }
            data->new_baud_rate = speed;

            debug_pr("speed: %" PRIu32 "\n", speed);

            break;
        case EV3_UART_CMD_WRITE:
            if (cmd2 & 0x20) {
                data->write_cmd_size = cmd2 & 0x3;
                if (data->type_id == PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR ||
                    data->type_id == PBIO_IODEV_TYPE_ID_CPLUS_L_MOTOR ||
                    data->type_id == PBIO_IODEV_TYPE_ID_CPLUS_XL_MOTOR) {
                    // TODO: msg[3] and msg[4] probably give us useful information
                    data->iodev.flags |= PBIO_IODEV_FLAG_IS_MOTOR;
                    // FIXME: clear this flag when device disconnects
                }
                else {
                    // TODO: handle other write commands
                }
            }
            break;
        case EV3_UART_CMD_EXT_MODE:
            // Powered up devices can have modes > EV3_UART_MODE_MAX. This
            // command precedes other commands to add the extra 8 to the mode
            data->ext_mode = data->rx_msg[1];
            break;
        case EV3_UART_CMD_VERSION:
            if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_VERSION, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate version INFO");
                goto err;
            }
            // TODO: this might be useful someday
            debug_pr("fw version: %08" PRIx32 "\n", uint32_le(data->rx_msg + 1));
            debug_pr("hw version: %08" PRIx32 "\n", uint32_le(data->rx_msg + 5));
            break;
        default:
            DBG_ERR(data->last_err = "Unknown command");
            goto err;
        }
        break;
    case EV3_UART_MSG_TYPE_INFO:
        switch (cmd2) {
        case EV3_UART_INFO_NAME:
            data->info_flags &= ~EV3_UART_INFO_FLAG_ALL_INFO;
            if (data->rx_msg[2] < 'A' || data->rx_msg[2] > 'z') {
                DBG_ERR(data->last_err = "Invalid name INFO");
                goto err;
            }
            /*
             * Name may not have null terminator and we
             * are done with the checksum at this point
             * so we are writing 0 over the checksum to
             * ensure a null terminator for the string
             * functions.
             */
            data->rx_msg[msg_size - 1] = 0;
            if (strlen((char *)data->rx_msg + 2) > PBIO_IODEV_MODE_NAME_SIZE) {
                DBG_ERR(data->last_err = "Name is too long");
                goto err;
            }
            snprintf(data->info->mode_info[mode].name,
                        PBIO_IODEV_MODE_NAME_SIZE + 1, "%s",
                        data->rx_msg + 2);
            data->new_mode = mode;
            data->info_flags |= EV3_UART_INFO_FLAG_INFO_NAME;

            debug_pr("new_mode: %d\n", data->new_mode);
            debug_pr("name: %s\n", data->info->mode_info[mode].name);

            break;
        case EV3_UART_INFO_RAW:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_RAW, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate raw scaling INFO");
                goto err;
            }
            data->info->mode_info[mode].raw_min = float_le(data->rx_msg + 2);
            data->info->mode_info[mode].raw_max = float_le(data->rx_msg + 6);

            debug_pr("raw: %f %f\n", (double)data->info->mode_info[mode].raw_min,
                                     (double)data->info->mode_info[mode].raw_max);

            break;
        case EV3_UART_INFO_PCT:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_PCT, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate percent scaling INFO");
                goto err;
            }
            data->info->mode_info[mode].pct_min = float_le(data->rx_msg + 2);
            data->info->mode_info[mode].pct_max = float_le(data->rx_msg + 6);

            debug_pr("pct: %f %f\n", (double)data->info->mode_info[mode].pct_min,
                                     (double)data->info->mode_info[mode].pct_max);

            break;
        case EV3_UART_INFO_SI:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_SI,
                            &data->info_flags))
            {
                DBG_ERR(data->last_err = "Received duplicate SI scaling INFO");
                goto err;
            }
            data->info->mode_info[mode].si_min = float_le(data->rx_msg + 2);
            data->info->mode_info[mode].si_max = float_le(data->rx_msg + 6);

            debug_pr("si: %f %f\n", (double)data->info->mode_info[mode].si_min,
                                    (double)data->info->mode_info[mode].si_max);

            break;
        case EV3_UART_INFO_UNITS:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNITS, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate SI units INFO");
                goto err;
            }
            // Units may not have null terminator and we are done with the
            // checksum at this point so we are writing 0 over the checksum to
            // ensure a null terminator for the string functions.
            data->rx_msg[msg_size - 1] = 0;
            snprintf(data->info->mode_info[mode].uom, PBIO_IODEV_UOM_SIZE + 1,
                     "%s", data->rx_msg + 2);

            debug_pr("uom: %s\n", data->info->mode_info[mode].uom);

            break;
        case EV3_UART_INFO_MAPPING:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MAPPING, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate mapping INFO");
                goto err;
            }

            data->info->mode_info[mode].input_flags = data->rx_msg[2];
            data->info->mode_info[mode].output_flags = data->rx_msg[3];

            debug_pr("mapping: in %02x out %02x\n", data->rx_msg[2], data->rx_msg[3]);

            break;
        case EV3_UART_INFO_MODE_COMBOS:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MODE_COMBOS, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate mode combos INFO");
                goto err;
            }
            debug_pr("mode combos: %04x\n", data->rx_msg[3] << 8 | data->rx_msg[2]);
            // TODO: this is actually array of values

            break;
        case EV3_UART_INFO_MOTOR_BIAS:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MOTOR_BIAS, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate motor bias INFO");
                goto err;
            }
            // TODO: do we need to store this info?

            debug_pr("motor bias: %02x\n", data->rx_msg[2]);

            break;
        case EV3_UART_INFO_CAPABILITY:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_CAPABILITY, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate capability INFO");
                goto err;
            }
            // TODO: do we need to store this info?

            debug_pr("capability: %02x %02x %02x %02x %02x %02x\n",
                data->rx_msg[2], data->rx_msg[3], data->rx_msg[4],
                data->rx_msg[5], data->rx_msg[6], data->rx_msg[7]);

            break;
        case EV3_UART_INFO_FORMAT:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_FORMAT, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate format INFO");
                goto err;
            }
            data->info->mode_info[mode].num_values = data->rx_msg[2];
            if (!data->info->mode_info[mode].num_values) {
                DBG_ERR(data->last_err = "Invalid number of data sets");
                goto err;
            }
            if (msg_size < 7) {
                DBG_ERR(data->last_err = "Invalid format data->msg size");
                goto err;
            }
            if ((data->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                DBG_ERR(data->last_err = "Did not receive all required INFO");
                goto err;
            }
            switch (data->rx_msg[3]) {
            case EV3_UART_DATA_8:
                data->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_INT8;
                break;
            case EV3_UART_DATA_16:
                data->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_INT16;
                break;
            case EV3_UART_DATA_32:
                data->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_INT32;
                break;
            case EV3_UART_DATA_FLOAT:
                data->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_FLOAT;
                break;
            default:
                DBG_ERR(data->last_err = "Invalid data type");
                goto err;
            }
            data->info->mode_info[mode].digits = data->rx_msg[4];
            data->info->mode_info[mode].decimals = data->rx_msg[5];
            if (data->new_mode) {
                data->new_mode--;
            }

            debug_pr("num_values: %d\n", data->info->mode_info[mode].num_values);
            debug_pr("data_type: %d\n", data->info->mode_info[mode].data_type);
            debug_pr("digits: %d\n", data->info->mode_info[mode].digits);
            debug_pr("decimals: %d\n", data->info->mode_info[mode].decimals);

            break;
        }
        break;
    case EV3_UART_MSG_TYPE_DATA:
        if (data->status != PBIO_UARTDEV_STATUS_DATA) {
            DBG_ERR(data->last_err = "Received DATA before INFO was complete");
            goto err;
        }

        if ((data->type_id == PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR ||
             data->type_id == PBIO_IODEV_TYPE_ID_CPLUS_L_MOTOR ||
             data->type_id == PBIO_IODEV_TYPE_ID_CPLUS_XL_MOTOR) && data->write_cmd_size > 0) {
            data->tacho_rate = data->rx_msg[1];
            data->tacho_count = uint32_le(data->rx_msg + 2);
        }
        else {
            if (mode >= data->info->num_modes) {
                DBG_ERR(data->last_err = "Invalid mode received");
                goto err;
            }
            if (mode != data->iodev.mode) {
                if (mode == data->new_mode) {
                    data->iodev.mode = mode;
                    // TODO: notify that mode has changed
                } else {
                    DBG_ERR(data->last_err = "Unexpected mode");
                    goto err;
                }
            }
            memcpy(data->iodev.bin_data, data->rx_msg + 1, msg_size - 2);
        }

        data->data_rec = 1;
        if (data->num_data_err) {
            data->num_data_err--;
        }
        break;
    }

    return;

err:
    // FIXME: Setting status to ERR here does not allow recovering from bad
    // message when receiving data. Maybe return error instead?
    data->status = PBIO_UARTDEV_STATUS_ERR;
}

static uint8_t ev3_uart_set_msg_hdr(enum ev3_uart_msg_type type, enum ev3_uart_msg_size size, enum ev3_uart_cmd cmd)
{
    return (type & EV3_UART_MSG_TYPE_MASK) | (size & EV3_UART_MSG_SIZE_MASK) | (cmd & EV3_UART_MSG_CMD_MASK);
}

static pbio_error_t ev3_uart_begin_tx_msg(uartdev_port_data_t *port_data, enum ev3_uart_msg_type msg_type,
                                          enum ev3_uart_cmd cmd, const uint8_t *data, uint8_t len) {
    uint8_t header, checksum, i;
    uint8_t offset = 0;
    enum ev3_uart_msg_size size;
    pbio_error_t err;

    if (len == 0 || len > 32) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (port_data->status != PBIO_UARTDEV_STATUS_DATA) {
        return PBIO_ERROR_NO_DEV;
    }

    if (port_data->tx_busy) {
        return PBIO_ERROR_AGAIN;
    }

    port_data->tx_busy = true;

    if (msg_type == EV3_UART_MSG_TYPE_DATA) {
        // Only Powered Up devices support setting data, and they expect to have an
        // extra command sent to give the part of the mode > 7
        port_data->tx_msg[0] = ev3_uart_set_msg_hdr(EV3_UART_MSG_TYPE_CMD, EV3_UART_MSG_SIZE_1, EV3_UART_CMD_EXT_MODE);
        port_data->tx_msg[1] = port_data->iodev.mode > EV3_UART_MODE_MAX ? 8 : 0;
        port_data->tx_msg[2] = 0xff ^ port_data->tx_msg[0] ^ port_data->tx_msg[1];
        offset = 3;
    }

    checksum = 0xff;
    for (i = 0; i < len; i++) {
        port_data->tx_msg[offset + i + 1] = data[i];
        checksum ^= data[i];
    }

    // can't send arbitrary number of bytes, so find nearest match
    if (i == 1) {
        size = EV3_UART_MSG_SIZE_1;
    }
    else if (i == 2) {
        size = EV3_UART_MSG_SIZE_2;
    }
    else if (i <= 4) {
        size = EV3_UART_MSG_SIZE_4;
        len = 4;
    }
    else if (i <= 8) {
        size = EV3_UART_MSG_SIZE_8;
        len = 8;
    }
    else if (i <= 16) {
        size = EV3_UART_MSG_SIZE_16;
        len = 16;
    }
    else if (i <= 32) {
        size = EV3_UART_MSG_SIZE_32;
        len = 32;
    }

    // pad with zeros
    for (; i < len; i++) {
        port_data->tx_msg[offset + i + 1] = 0;
    }

    header = ev3_uart_set_msg_hdr(msg_type, size, cmd);
    checksum ^= header;

    port_data->tx_msg[offset] = header;
    port_data->tx_msg[offset + i + 1] = checksum;

    err = pbdrv_uart_write_begin(port_data->uart, port_data->tx_msg, offset + i + 2, EV3_UART_IO_TIMEOUT);
    if (err != PBIO_SUCCESS) {
        port_data->tx_busy = false;
    }

    return err;
}

static PT_THREAD(pbio_uartdev_update(uartdev_port_data_t *data)) {
    pbio_error_t err;
    uint8_t checksum;

    PT_BEGIN(&data->pt);

    // reset state for new device
    data->info->type_id = PBIO_IODEV_TYPE_ID_NONE;
    data->ext_mode = 0;
    data->status = PBIO_UARTDEV_STATUS_SYNCING;
    PBIO_PT_WAIT_READY(&data->pt, pbdrv_uart_set_baud_rate(data->uart, EV3_UART_SPEED_MIN));

    // To get in sync with the data stream from the sensor, we look for a valid TYPE command.
    for (;;) {
        PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_begin(data->uart, data->rx_msg, 1, EV3_UART_IO_TIMEOUT));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx error during sync");
            goto err;
        }
        PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_end(data->uart));
        if (err == PBIO_ERROR_TIMEDOUT) {
            continue;
        }
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx error during sync");
            goto err;
        }

        if (data->rx_msg[0] == (EV3_UART_MSG_TYPE_CMD | EV3_UART_CMD_TYPE)) {
            break;
        }
    }

    // then read the rest of the message

    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_begin(data->uart, data->rx_msg + 1, 2, EV3_UART_IO_TIMEOUT));
    if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "UART Rx error while reading type");
        goto err;
    }
    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_end(data->uart));
    if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "UART Rx error while reading type");
        goto err;
    }

    if (data->rx_msg[0] < EV3_UART_TYPE_MIN || data->rx_msg[0] > EV3_UART_TYPE_MAX) {
        DBG_ERR(data->last_err = "Bad device type id");
        goto err;
    }

    checksum = 0xff ^ data->rx_msg[0] ^ data->rx_msg[1];
    if (data->rx_msg[2] != checksum) {
        DBG_ERR(data->last_err = "Bad checksum for type id");
        goto err;
    }

    // if all was good, we are ready to start receiving the mode info

    data->info->num_modes = 1;
    data->info->num_view_modes = 1;

    for (int i = 0; i < PBIO_IODEV_MAX_NUM_MODES; i++) {
        data->info->mode_info[i] = ev3_uart_default_mode_info;
    }

    data->type_id = data->rx_msg[1];
    data->info_flags = EV3_UART_INFO_FLAG_CMD_TYPE;
    data->data_rec = 0;
    data->num_data_err = 0;
    data->status = PBIO_UARTDEV_STATUS_INFO;
    debug_pr("type id: %d\n", data->type_id);

    while (data->status == PBIO_UARTDEV_STATUS_INFO) {
        // read the message header
        PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_begin(data->uart, data->rx_msg, 1, EV3_UART_IO_TIMEOUT));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx begin error during info header");
            goto err;
        }
        PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_end(data->uart));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx end error during info header");
            goto err;
        }

        data->rx_msg_size = ev3_uart_get_msg_size(data->rx_msg[0]);
        if (data->rx_msg_size > EV3_UART_MAX_MESSAGE_SIZE) {
            DBG_ERR(data->last_err = "Bad message size during info");
            goto err;
        }

        // read the rest of the message
        if (data->rx_msg_size > 1) {
            PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_begin(data->uart, data->rx_msg + 1, data->rx_msg_size - 1, EV3_UART_IO_TIMEOUT));
            if (err != PBIO_SUCCESS) {
                DBG_ERR(data->last_err = "UART Rx begin error during info");
                goto err;
            }
            PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_end(data->uart));
            if (err != PBIO_SUCCESS) {
                DBG_ERR(data->last_err = "UART Rx end error during info");
                goto err;
            }
        }

        // at this point, we have a full data->msg that can be parsed
        pbio_uartdev_parse_msg(data);
    }

    // at this point we should have read all of the mode info
    if (data->status != PBIO_UARTDEV_STATUS_ACK) {
        // data->last_err should be set by pbio_uartdev_parse_msg()
        goto err;
    }

    // reply with ACK
    PT_WAIT_WHILE(&data->pt, data->tx_busy);
    data->tx_busy = true;
    data->tx_msg[0] = EV3_UART_SYS_ACK;
    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_write_begin(data->uart, data->tx_msg, 1, EV3_UART_IO_TIMEOUT));
    if (err != PBIO_SUCCESS) {
        data->tx_busy = false;
        DBG_ERR(data->last_err = "UART Tx begin error during ack");
        goto err;
    }
    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_write_end(data->uart));
    if (err != PBIO_SUCCESS) {
        data->tx_busy = false;
        DBG_ERR(data->last_err = "UART Tx end error during ack");
        goto err;
    }
    data->tx_busy = false;

    // schedule baud rate change
    etimer_set(&data->timer, clock_from_msec(10));
    PT_WAIT_UNTIL(&data->pt, etimer_expired(&data->timer));

    // change the baud rate
    PBIO_PT_WAIT_READY(&data->pt, pbdrv_uart_set_baud_rate(data->uart, data->new_baud_rate));

    // setting type_id in info struct lets external modules know a device is connected
    data->info->type_id = data->type_id;
    data->status = PBIO_UARTDEV_STATUS_DATA;
    // reset data rx thread
    PT_INIT(&data->data_pt);

    if (data->type_id == PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR ||
        data->type_id == PBIO_IODEV_TYPE_ID_CPLUS_L_MOTOR ||
        data->type_id == PBIO_IODEV_TYPE_ID_CPLUS_XL_MOTOR) {
        static const uint8_t mode_1_and_2_combo[] = {
            0x20 | 2,   // modo combo command, 2 modes
            0,          // combo index
            1 << 4 | 0, // mode 1, dataset 0
            2 << 4 | 0, // mode 2, dataset 0
        };

        // setup motor to send position and speed data
        PBIO_PT_WAIT_READY(&data->pt,
            err = ev3_uart_begin_tx_msg(data, EV3_UART_MSG_TYPE_CMD, EV3_UART_CMD_WRITE,
                mode_1_and_2_combo, PBIO_ARRAY_SIZE(mode_1_and_2_combo)));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Tx begin error during motor");
            goto err;
        }
        PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_write_end(data->uart));
        if (err != PBIO_SUCCESS) {
            data->tx_busy = false;
            DBG_ERR(data->last_err = "UART Tx end error during motor");
            goto err;
        }
        data->tx_busy = false;
    }

    while (data->status == PBIO_UARTDEV_STATUS_DATA) {
        // setup keepalive timer
        etimer_reset_with_new_interval(&data->timer, clock_from_msec(EV3_UART_DATA_KEEP_ALIVE_TIMEOUT));
        PT_WAIT_UNTIL(&data->pt, etimer_expired(&data->timer));

        // make sure we are receiving data
        if (!data->data_rec) {
            data->num_data_err++;
            DBG_ERR(data->last_err = "No data since last keepalive");
            if (data->num_data_err > 6) {
                data->status = PBIO_UARTDEV_STATUS_ERR;
            }
        }
        data->data_rec = 0;

        // send keepalive
        PT_WAIT_WHILE(&data->pt, data->tx_busy);
        data->tx_busy = true;
        data->tx_msg[0] = EV3_UART_SYS_NACK;
        PBIO_PT_WAIT_READY(&data->pt,
            err = pbdrv_uart_write_begin(data->uart, data->tx_msg, 1, EV3_UART_IO_TIMEOUT));
        if (err != PBIO_SUCCESS) {
            data->tx_busy = false;
            DBG_ERR(data->last_err = "UART Tx begin error during keepalive");
            goto err;
        }
        PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_write_end(data->uart));
        if (err != PBIO_SUCCESS) {
            data->tx_busy = false;
            DBG_ERR(data->last_err = "UART Tx end error during keepalive");
            goto err;
        }
        data->tx_busy = false;
    }

err:
    // reset and start over
    data->status = PBIO_UARTDEV_STATUS_ERR;
    etimer_stop(&data->timer);
    debug_pr("%s\n", data->last_err);
    data->err_count++;

    process_post(PROCESS_BROADCAST, PROCESS_EVENT_SERVICE_REMOVED, NULL);

    PT_END(&data->pt);
}

// REVISIT: This is not the greatest. We can easily get a buffer overrun and
// loose data. For now, the retry after bad message size helps get back into
// sync with the data stream.
static PT_THREAD(pbio_uartdev_receive_data(uartdev_port_data_t *data)) {
    pbio_error_t err;

    PT_BEGIN(&data->data_pt);

    while (true) {
        PBIO_PT_WAIT_READY(&data->data_pt,
            err = pbdrv_uart_read_begin(data->uart, data->rx_msg, 1, EV3_UART_IO_TIMEOUT));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx data header begin error");
            break;
        }
        PBIO_PT_WAIT_READY(&data->data_pt, err = pbdrv_uart_read_end(data->uart));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx data header end error");
            break;
        }

        data->rx_msg_size = ev3_uart_get_msg_size(data->rx_msg[0]);
        if (data->rx_msg_size < 3 || data->rx_msg_size > EV3_UART_MAX_MESSAGE_SIZE) {
            DBG_ERR(data->last_err = "Bad data message size");
            continue;
        }
        if ((data->rx_msg[0] & EV3_UART_MSG_TYPE_MASK) != EV3_UART_MSG_TYPE_DATA &&
            (data->rx_msg[0] & (EV3_UART_MSG_TYPE_MASK | EV3_UART_MSG_CMD_MASK)) != (EV3_UART_MSG_TYPE_CMD | EV3_UART_CMD_WRITE)) {
            DBG_ERR(data->last_err = "Bad msg type");
            continue;
        }

        PBIO_PT_WAIT_READY(&data->data_pt,
            err = pbdrv_uart_read_begin(data->uart, data->rx_msg + 1, data->rx_msg_size - 1, EV3_UART_IO_TIMEOUT));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx data begin error");
            break;
        }
        PBIO_PT_WAIT_READY(&data->data_pt, err = pbdrv_uart_read_end(data->uart));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(data->last_err = "UART Rx data end error");
            break;
        }

        // at this point, we have a full data->msg that can be parsed
        pbio_uartdev_parse_msg(data);
    }

    PT_END(&data->data_pt);
}

static pbio_error_t ev3_uart_set_mode_begin(pbio_iodev_t *iodev, uint8_t mode) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    pbio_error_t err;

    err = ev3_uart_begin_tx_msg(port_data, EV3_UART_MSG_TYPE_CMD, EV3_UART_CMD_SELECT, &mode, 1);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    port_data->new_mode = mode;

    return PBIO_SUCCESS;
}

static pbio_error_t ev3_uart_set_mode_end(pbio_iodev_t *iodev) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    pbio_error_t err;

    err = pbdrv_uart_write_end(port_data->uart);
    if (err != PBIO_ERROR_AGAIN) {
        port_data->tx_busy = false;
        // TODO: should wait until we receive at least one data message to
        // ensure that the mode has actually changed (also ensures that we have
        // a new data value in the case of single shot modes)
    }

    return err;
}

static pbio_error_t ev3_uart_set_data_begin(pbio_iodev_t *iodev, const uint8_t *data) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    pbio_iodev_mode_t *mode = &port_data->info->mode_info[iodev->mode];
    uint8_t size;

    // not all modes support setting data
    if (!mode->output_flags) {
        return PBIO_ERROR_INVALID_OP;
    }

    size = mode->num_values * pbio_iodev_size_of(mode->data_type);

    return ev3_uart_begin_tx_msg(port_data, EV3_UART_MSG_TYPE_DATA, iodev->mode, data, size);
}

static pbio_error_t ev3_uart_write_begin(pbio_iodev_t *iodev, const uint8_t *data, uint8_t size) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);

    return ev3_uart_begin_tx_msg(port_data, EV3_UART_MSG_TYPE_CMD, EV3_UART_CMD_WRITE, data, size);
}

static pbio_error_t ev3_uart_write_end(pbio_iodev_t *iodev) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    pbio_error_t err;

    err = pbdrv_uart_write_end(port_data->uart);
    if (err != PBIO_ERROR_AGAIN) {
        port_data->tx_busy = false;
    }

    return err;
}

static void ev3_uart_write_cancel(pbio_iodev_t *iodev) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);

    pbdrv_uart_write_cancel(port_data->uart);
}

static const pbio_iodev_ops_t pbio_uartdev_ops = {
    .set_mode_begin = ev3_uart_set_mode_begin,
    .set_mode_end = ev3_uart_set_mode_end,
    .set_mode_cancel = ev3_uart_write_cancel,
    .set_data_begin = ev3_uart_set_data_begin,
    .set_data_end = ev3_uart_write_end,
    .set_data_cancel = ev3_uart_write_cancel,
    .write_begin = ev3_uart_write_begin,
    .write_end = ev3_uart_write_end,
    .write_cancel = ev3_uart_write_cancel,
};

static pbio_error_t pbio_uartdev_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(dev, uartdev_port_data_t, counter_dev);

    if (port_data->info->type_id != PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR &&
        port_data->info->type_id != PBIO_IODEV_TYPE_ID_CPLUS_L_MOTOR &&
        port_data->info->type_id != PBIO_IODEV_TYPE_ID_CPLUS_XL_MOTOR) {
        return PBIO_ERROR_NO_DEV;
    }

    *count = port_data->tacho_count;

    return PBIO_SUCCESS;
}

static pbio_error_t pbio_uartdev_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(dev, uartdev_port_data_t, counter_dev);

    if (port_data->info->type_id != PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR &&
        port_data->info->type_id != PBIO_IODEV_TYPE_ID_CPLUS_L_MOTOR &&
        port_data->info->type_id != PBIO_IODEV_TYPE_ID_CPLUS_XL_MOTOR) {
        return PBIO_ERROR_NO_DEV;
    }

    // UART motors return speed in % of max speed, so we have to adjust it to
    // counts per second.
    // scaling factor of 14 determined empirically for BOOST Interactive motor
    // TODO: scale rate based on individual motor type.
    *rate = port_data->tacho_rate * 14;

    return PBIO_SUCCESS;
}

static PT_THREAD(pbio_uartdev_init(struct pt *pt, uint8_t id)) {
    const pbio_uartdev_platform_data_t *pdata = &pbio_uartdev_platform_data[id];
    uartdev_port_data_t *port_data = &dev_data[id];

    PT_BEGIN(pt);

    PT_WAIT_UNTIL(pt, pbdrv_uart_get(pdata->uart_id, &port_data->uart) == PBIO_SUCCESS);
    port_data->iodev.info = &infos[id].info;
    port_data->iodev.ops = &pbio_uartdev_ops;
    port_data->counter_dev.get_count = pbio_uartdev_get_count;
    port_data->counter_dev.get_rate = pbio_uartdev_get_rate;
    port_data->counter_dev.initalized = true;
    port_data->info =  &infos[id].info;
    port_data->tx_msg = &bufs[id][BUF_TX_MSG][0];
    port_data->rx_msg = &bufs[id][BUF_RX_MSG][0];

    pbdrv_counter_register(pdata->counter_id, &port_data->counter_dev);

    PT_END(pt);
}

PROCESS_THREAD(pbio_uartdev_process, ev, data) {
    static struct pt pt;
    static int i;

    PROCESS_BEGIN();

    for (i = 0; i < PBIO_CONFIG_UARTDEV_NUM_DEV; i++) {
        PROCESS_PT_SPAWN(&pt, pbio_uartdev_init(&pt, i));
    }

    while (true) {
        for (i = 0; i < PBIO_CONFIG_UARTDEV_NUM_DEV; i++) {
            pbio_uartdev_update(&dev_data[i]);
            if (dev_data[i].status == PBIO_UARTDEV_STATUS_DATA) {
                pbio_uartdev_receive_data(&dev_data[i]);
            }
        }
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_UARTDEV

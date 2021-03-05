// SPDX-License-Identifier: MIT OR GPL-2.0-only
// Copyright (c) 2018-2020 The Pybricks Authors

/*
 * Based on:
 * LEGO MINDSTORMS EV3 UART Sensor tty line discipline
 *
 * Copyright (c) 2014-2016,2018-2019 David Lechner <david@lechnology.com>
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
#include <lego_uart.h>

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
#include <pbdrv/motor.h>

#define EV3_UART_MAX_MESSAGE_SIZE   (LUMP_MAX_MSG_SIZE + 3)

#define EV3_UART_MAX_DATA_ERR       6

#define EV3_UART_TYPE_MIN           29      // EV3 color sensor
#define EV3_UART_TYPE_MAX           101
#define EV3_UART_SPEED_MIN          2400
#define EV3_UART_SPEED_LPF2         115200  // standard baud rate for Powered Up
#define EV3_UART_SPEED_MAX          460800  // in practice 115200 is max

#define EV3_UART_DATA_KEEP_ALIVE_TIMEOUT    100 /* msec */
#define EV3_UART_IO_TIMEOUT                 250 /* msec */

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
    EV3_UART_INFO_BIT_INFO_UNK7,
    EV3_UART_INFO_BIT_INFO_UNK8,
    EV3_UART_INFO_BIT_INFO_UNK9,
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
    EV3_UART_INFO_FLAG_INFO_UNK7                = 1 << EV3_UART_INFO_BIT_INFO_UNK7,
    EV3_UART_INFO_FLAG_INFO_UNK8                = 1 << EV3_UART_INFO_BIT_INFO_UNK8,
    EV3_UART_INFO_FLAG_INFO_UNK9                = 1 << EV3_UART_INFO_BIT_INFO_UNK9,
    EV3_UART_INFO_FLAG_INFO_FORMAT              = 1 << EV3_UART_INFO_BIT_INFO_FORMAT,

    EV3_UART_INFO_FLAG_ALL_INFO     = EV3_UART_INFO_FLAG_INFO_NAME
        | EV3_UART_INFO_FLAG_INFO_RAW
        | EV3_UART_INFO_FLAG_INFO_PCT
        | EV3_UART_INFO_FLAG_INFO_SI
        | EV3_UART_INFO_FLAG_INFO_UNITS
        | EV3_UART_INFO_FLAG_INFO_MAPPING
        | EV3_UART_INFO_FLAG_INFO_MODE_COMBOS
        | EV3_UART_INFO_FLAG_INFO_UNK7
        | EV3_UART_INFO_FLAG_INFO_UNK8
        | EV3_UART_INFO_FLAG_INFO_UNK9
        | EV3_UART_INFO_FLAG_INFO_FORMAT,
    EV3_UART_INFO_FLAG_REQUIRED     = EV3_UART_INFO_FLAG_CMD_TYPE
        | EV3_UART_INFO_FLAG_CMD_MODES
        | EV3_UART_INFO_FLAG_INFO_NAME
        | EV3_UART_INFO_FLAG_INFO_FORMAT,
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
 * @pt: Protothread for main communication protocol
 * @data_pt: Protothread for receiving sensor data
 * @speed_pt: Protothread for setting the baud rate
 * @timer: Timer for sending keepalive messages and other delays.
 * @uart: Pointer to the UART device to use for communications
 * @info: The I/O device information struct for the connected device
 * @status: The current device connection state
 * @type_id: The type ID received
 * @requested_mode: Mode that was requested by user. Used to restore previous
 *      mode in case of a reconnect.
 * @new_mode: The mode requested by set_mode. Also used to keep track of mode
 *  in INFO messages while syncing.
 * @new_baud_rate: New baud rate that will be set with ev3_uart_change_bitrate
 * @info_flags: Flags indicating what information has already been read
 *      from the data.
 * @tacho_count: The tacho count received from an LPF2 motor
 * @tacho_offset: Tacho count offset to account for unexpected jumps in LPF2 tacho data
 * @abs_pos: The absolute position received from an LPF2 motor
 * @tx_msg: Buffer to hold messages transmitted to the device
 * @rx_msg: Buffer to hold messages received from the device
 * @rx_msg_size: Size of the current message being received
 * @ext_mode: Extra mode adder for Powered Up devices (for modes > LUMP_MAX_MODE)
 * @write_cmd_size: The size parameter received from a WRITE command
 * @tacho_rate: The tacho rate received from an LPF2 motor
 * @max_tacho_rate: The "100%" rate received from an LPF2 motor
 * @last_err: data->msg to be printed in case of an error.
 * @err_count: Total number of errors that have occurred
 * @num_data_err: Number of bad reads when receiving DATA data->msgs.
 * @data_rec: Flag that indicates that good DATA data->msg has been received
 *      since last watchdog timeout.
 * @tx_busy: mutex that protects tx_msg
 * @mode_change_tx_done: Flag to keep ev3_uart_set_mode_end() blocked until
 * mode has actually changed
 * @speed_payload: Buffer for holding baud rate change message data
 * @mode_combo_payload: Buffer for holding mode combo message data
 * @mode_combo_size: Actual size of mode combo message
 */
typedef struct {
    pbio_iodev_t iodev;
    struct pt pt;
    struct pt data_pt;
    struct pt speed_pt;
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
    int32_t tacho_offset;
    int16_t abs_pos;
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t rx_msg_size;
    uint8_t ext_mode;
    uint8_t write_cmd_size;
    int8_t tacho_rate;
    int32_t max_tacho_rate;
    DBG_ERR(const char *last_err);
    uint32_t err_count;
    uint32_t num_data_err;
    bool data_rec;
    bool tx_busy;
    bool mode_change_tx_done;
    uint8_t speed_payload[4];
    uint8_t mode_combo_payload[5];
    uint8_t mode_combo_size;
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

static pbdrv_counter_dev_t *counter_devs;

static const pbio_iodev_mode_t ev3_uart_default_mode_info = {
    .raw_max = 1023,
    .pct_max = 100,
    .si_max = 1,
    .digits = 4,
};

#define PBIO_PT_WAIT_READY(pt, expr) PT_WAIT_UNTIL((pt), (expr) != PBIO_ERROR_AGAIN)

pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev) {
    if (id >= PBIO_CONFIG_UARTDEV_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *iodev = &dev_data[id].iodev;

    if (!(*iodev)->info) {
        // device has not been initalized yet
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

static inline float float_le(uint8_t *bytes) {
    union {
        float f;
        uint32_t u;
    } result;

    result.u = pbio_get_uint32_le(bytes);

    return result.f;
}

static inline bool test_and_set_bit(uint8_t bit, uint32_t *flags) {
    bool result = *flags & (1 << bit);
    *flags |= (1 << bit);
    return result;
}

static uint8_t ev3_uart_get_msg_size(uint8_t header) {
    uint8_t size;

    if ((header & LUMP_MSG_TYPE_MASK) == LUMP_MSG_TYPE_SYS) {
        return 1;
    }

    size = LUMP_MSG_SIZE(header);
    size += 2; /* header and checksum */
    if ((header & LUMP_MSG_TYPE_MASK) == LUMP_MSG_TYPE_INFO) {
        size++; /* extra command byte */

    }
    return size;
}

static void pbio_uartdev_set_mode_flags(pbio_iodev_type_id_t type_id, uint8_t mode, lump_mode_flags_t *flags) {
    memset(flags, 0, sizeof(*flags));

    switch (type_id) {
        case PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR:
            switch (mode) {
                case 0: // POWER
                    flags->flags0 = LUMP_MODE_FLAGS0_MOTOR_POWER | LUMP_MODE_FLAGS0_MOTOR;
                    break;
                case 1: // SPEED
                    flags->flags0 = LUMP_MODE_FLAGS0_MOTOR_SPEED | LUMP_MODE_FLAGS0_MOTOR;
                    break;
                case 2: // POS
                    flags->flags0 = LUMP_MODE_FLAGS0_MOTOR_REL_POS | LUMP_MODE_FLAGS0_MOTOR;
                    break;
            }
            flags->flags4 = LUMP_MODE_FLAGS4_USES_HBRIDGE;
            flags->flags5 = LUMP_MODE_FLAGS5_UNKNOWN_BIT1; // TODO: figure out what this flag means
            break;
        default:
            break;
    }
}

static void pbio_uartdev_parse_msg(uartdev_port_data_t *data) {
    uint32_t speed;
    uint8_t msg_type, cmd, msg_size, mode, cmd2;

    msg_type = data->rx_msg[0] & LUMP_MSG_TYPE_MASK;
    cmd = data->rx_msg[0] & LUMP_MSG_CMD_MASK;
    msg_size = ev3_uart_get_msg_size(data->rx_msg[0]);
    mode = cmd;
    cmd2 = data->rx_msg[1];

    // The original EV3 spec only allowed for up to 8 modes (3-bit number).
    // The Powered UP spec extents this by adding an extra flag to INFO commands.
    // Not sure that LUMP_INFO_MODE_PLUS_8 is used in practice, but rather
    // an extra (separate) LUMP_CMD_EXT_MODE message seems to be used instead
    if (msg_type == LUMP_MSG_TYPE_INFO && (cmd2 & LUMP_INFO_MODE_PLUS_8)) {
        mode += 8;
        cmd2 &= ~LUMP_INFO_MODE_PLUS_8;
    } else {
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
                    || data->rx_msg[0] != (LUMP_MSG_TYPE_DATA | LUMP_MSG_SIZE_8 | 4)) {
                    return;
                }
            } else {
                goto err;
            }
        }
    }

    switch (msg_type) {
        case LUMP_MSG_TYPE_SYS:
            switch (cmd) {
                case LUMP_SYS_SYNC:
                    /* IR sensor (type 33) sends checksum after SYNC */
                    if (msg_size > 1 && (cmd ^ cmd2) == 0xFF) {
                        msg_size++;
                    }
                    break;
                case LUMP_SYS_ACK:
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
        case LUMP_MSG_TYPE_CMD:
            switch (cmd) {
                case LUMP_CMD_MODES:
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_MODES, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate modes INFO");
                        goto err;
                    }
                    if (cmd2 > LUMP_MAX_MODE) {
                        DBG_ERR(data->last_err = "Number of modes is out of range");
                        goto err;
                    }
                    data->info->num_modes = cmd2 + 1;
                    if (msg_size > 5) {
                        // Powered Up devices can have an extended mode message that
                        // includes modes > LUMP_MAX_MODE
                        data->info->num_modes = data->rx_msg[3] + 1;
                        data->info->num_view_modes = data->rx_msg[4] + 1;
                    } else if (msg_size > 3) {
                        data->info->num_view_modes = data->rx_msg[2] + 1;
                    } else {
                        data->info->num_view_modes = data->info->num_modes;
                    }

                    debug_pr("num_modes: %d\n", data->info->num_modes);
                    debug_pr("num_view_modes: %d\n", data->info->num_view_modes);

                    break;
                case LUMP_CMD_SPEED:
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_SPEED, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate speed INFO");
                        goto err;
                    }
                    speed = pbio_get_uint32_le(data->rx_msg + 1);
                    if (speed < EV3_UART_SPEED_MIN || speed > EV3_UART_SPEED_MAX) {
                        DBG_ERR(data->last_err = "Speed is out of range");
                        goto err;
                    }
                    data->new_baud_rate = speed;

                    debug_pr("speed: %" PRIu32 "\n", speed);

                    break;
                case LUMP_CMD_WRITE:
                    if (cmd2 & 0x20) {
                        data->write_cmd_size = cmd2 & 0x3;
                        if (PBIO_IODEV_IS_FEEDBACK_MOTOR(&data->iodev)) {
                            // TODO: msg[3] and msg[4] probably give us useful information
                        } else {
                            // TODO: handle other write commands
                        }
                    }
                    break;
                case LUMP_CMD_EXT_MODE:
                    // Powered up devices can have modes > LUMP_MAX_MODE. This
                    // command precedes other commands to add the extra 8 to the mode
                    data->ext_mode = data->rx_msg[1];
                    break;
                case LUMP_CMD_VERSION:
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_VERSION, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate version INFO");
                        goto err;
                    }
                    // TODO: this might be useful someday
                    debug_pr("fw version: %08" PRIx32 "\n", pbio_get_uint32_le(data->rx_msg + 1));
                    debug_pr("hw version: %08" PRIx32 "\n", pbio_get_uint32_le(data->rx_msg + 5));
                    break;
                default:
                    DBG_ERR(data->last_err = "Unknown command");
                    goto err;
            }
            break;
        case LUMP_MSG_TYPE_INFO:
            switch (cmd2) {
                case LUMP_INFO_NAME: {
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
                    size_t name_len = strlen((char *)(data->rx_msg + 2));
                    if (name_len > LUMP_MAX_NAME_SIZE) {
                        DBG_ERR(data->last_err = "Name is too long");
                        goto err;
                    }
                    snprintf(data->info->mode_info[mode].name,
                        PBIO_IODEV_MODE_NAME_SIZE + 1, "%s", data->rx_msg + 2);
                    data->new_mode = mode;
                    data->info_flags |= EV3_UART_INFO_FLAG_INFO_NAME;

                    lump_mode_flags_t *flags = &data->info->mode_info[mode].flags;
                    if (name_len <= LUMP_MAX_SHORT_NAME_SIZE && msg_size > LUMP_MAX_NAME_SIZE) {
                        // newer LPF2 devices send additional mode flags along with the name
                        memcpy(flags, data->rx_msg + 8, 6);
                    } else {
                        // otherwise look up flags
                        pbio_uartdev_set_mode_flags(data->type_id, mode, flags);
                    }

                    if (flags->flags0 & LUMP_MODE_FLAGS0_MOTOR_POWER) {
                        data->iodev.capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_IS_MOTOR;
                    }
                    if (flags->flags0 & LUMP_MODE_FLAGS0_MOTOR_SPEED) {
                        data->iodev.capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_SPEED;
                    }
                    if (flags->flags0 & LUMP_MODE_FLAGS0_MOTOR_REL_POS) {
                        data->iodev.capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS;
                    }
                    if (flags->flags0 & LUMP_MODE_FLAGS0_MOTOR_ABS_POS) {
                        data->iodev.capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS;
                    }
                    if (flags->flags0 & LUMP_MODE_FLAGS0_REQUIRES_POWER) {
                        data->iodev.capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_REQUIRES_POWER;
                    }

                    debug_pr("new_mode: %d\n", data->new_mode);
                    debug_pr("name: %s\n", data->info->mode_info[mode].name);
                    debug_pr("flags: %02X %02X %02X %02X %02X %02X\n",
                        flags->flags0, flags->flags1, flags->flags2,
                        flags->flags3, flags->flags4, flags->flags5);
                }
                break;
                case LUMP_INFO_RAW:
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
                case LUMP_INFO_PCT:
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
                case LUMP_INFO_SI:
                    if (data->new_mode != mode) {
                        DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_SI,
                        &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate SI scaling INFO");
                        goto err;
                    }
                    data->info->mode_info[mode].si_min = float_le(data->rx_msg + 2);
                    data->info->mode_info[mode].si_max = float_le(data->rx_msg + 6);

                    debug_pr("si: %f %f\n", (double)data->info->mode_info[mode].si_min,
                        (double)data->info->mode_info[mode].si_max);

                    break;
                case LUMP_INFO_UNITS:
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
                case LUMP_INFO_MAPPING:
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
                case LUMP_INFO_MODE_COMBOS:
                    if (data->new_mode != mode) {
                        DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MODE_COMBOS, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate mode combos INFO");
                        goto err;
                    }

                    // REVISIT: this is potentially an array of combos
                    data->info->mode_combos = data->rx_msg[3] << 8 | data->rx_msg[2];

                    debug_pr("mode combos: %04x\n", data->info->mode_combos);

                    break;
                case LUMP_INFO_UNK9:
                    if (data->new_mode != mode) {
                        DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK9, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate UNK9 INFO");
                        goto err;
                    }

                    // first 3 parameters look like PID constants
                    data->max_tacho_rate = pbio_get_uint32_le(data->rx_msg + 14);

                    debug_pr("motor parameters: %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
                        pbio_get_uint32_le(data->rx_msg + 2), pbio_get_uint32_le(data->rx_msg + 6),
                        pbio_get_uint32_le(data->rx_msg + 10), data->max_tacho_rate);

                    break;
                case LUMP_INFO_FORMAT:
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
                    data->info->mode_info[mode].data_type = data->rx_msg[3];
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
        case LUMP_MSG_TYPE_DATA:
            if (data->status != PBIO_UARTDEV_STATUS_DATA) {
                DBG_ERR(data->last_err = "Received DATA before INFO was complete");
                goto err;
            }

            if (PBIO_IODEV_IS_FEEDBACK_MOTOR(&data->iodev) && data->write_cmd_size > 0) {
                data->tacho_rate = data->rx_msg[1];

                // Decode the tacho count data message
                int32_t tacho_count_msg = pbio_get_uint32_le(data->rx_msg + 2);

                // Sometimes, the incremental tacho data unexpectedly jumps by multiples
                // of -/+360, so add a correction if an impossibly high change is detected.
                while (tacho_count_msg - data->tacho_offset - data->tacho_count < -270) {
                    data->tacho_offset -= 360;
                }
                while (tacho_count_msg - data->tacho_offset - data->tacho_count > 270) {
                    data->tacho_offset += 360;
                }
                // The counter driver must return the corrected count
                data->tacho_count = tacho_count_msg - data->tacho_offset;

                if (data->iodev.capability_flags & PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS) {
                    data->abs_pos = data->rx_msg[7] << 8 | data->rx_msg[6];
                }
            } else {
                if (mode >= data->info->num_modes) {
                    DBG_ERR(data->last_err = "Invalid mode received");
                    goto err;
                }
                data->iodev.mode = mode;
                if (mode == data->new_mode) {
                    memcpy(data->iodev.bin_data, data->rx_msg + 1, msg_size - 2);
                }
            }

            data->data_rec = true;
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

static uint8_t ev3_uart_set_msg_hdr(lump_msg_type_t type, lump_msg_size_t size, lump_cmd_t cmd) {
    return (type & LUMP_MSG_TYPE_MASK) | (size & LUMP_MSG_SIZE_MASK) | (cmd & LUMP_MSG_CMD_MASK);
}

static pbio_error_t ev3_uart_begin_tx_msg(uartdev_port_data_t *port_data, lump_msg_type_t msg_type,
    lump_cmd_t cmd, const uint8_t *data, uint8_t len) {
    uint8_t header, checksum, i;
    uint8_t offset = 0;
    lump_msg_size_t size;
    pbio_error_t err;

    if (len == 0 || len > 32) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (port_data->tx_busy || port_data->mode_change_tx_done) {
        return PBIO_ERROR_AGAIN;
    }

    port_data->tx_busy = true;

    if (msg_type == LUMP_MSG_TYPE_DATA) {
        // Only Powered Up devices support setting data, and they expect to have an
        // extra command sent to give the part of the mode > 7
        port_data->tx_msg[0] = ev3_uart_set_msg_hdr(LUMP_MSG_TYPE_CMD, LUMP_MSG_SIZE_1, LUMP_CMD_EXT_MODE);
        port_data->tx_msg[1] = port_data->iodev.mode > LUMP_MAX_MODE ? 8 : 0;
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
        size = LUMP_MSG_SIZE_1;
    } else if (i == 2) {
        size = LUMP_MSG_SIZE_2;
    } else if (i <= 4) {
        size = LUMP_MSG_SIZE_4;
        len = 4;
    } else if (i <= 8) {
        size = LUMP_MSG_SIZE_8;
        len = 8;
    } else if (i <= 16) {
        size = LUMP_MSG_SIZE_16;
        len = 16;
    } else if (i <= 32) {
        size = LUMP_MSG_SIZE_32;
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

static PT_THREAD(pbio_uartdev_send_speed_msg(uartdev_port_data_t * data, uint32_t speed)) {
    pbio_error_t err;

    PT_BEGIN(&data->speed_pt);

    PT_WAIT_WHILE(&data->speed_pt, data->tx_busy);

    pbio_set_uint32_le(&data->speed_payload[0], speed);
    PBIO_PT_WAIT_READY(&data->speed_pt, err = ev3_uart_begin_tx_msg(data,
        LUMP_MSG_TYPE_CMD, LUMP_CMD_SPEED,
        data->speed_payload, PBIO_ARRAY_SIZE(data->speed_payload)));
    if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "SPEED tx begin failed");
        goto err;
    }

    PBIO_PT_WAIT_READY(&data->speed_pt, err = pbdrv_uart_write_end(data->uart));
    if (err != PBIO_SUCCESS) {
        data->tx_busy = false;
        DBG_ERR(data->last_err = "SPEED tx end failed");
        goto err;
    }

    data->tx_busy = false;

    PT_END(&data->speed_pt);

err:
    PT_EXIT(&data->speed_pt);
}

static PT_THREAD(pbio_uartdev_update(uartdev_port_data_t * data)) {
    pbio_error_t err;
    uint8_t checksum;

    PT_BEGIN(&data->pt);

    // TODO: wait for ioport to be ready for a uartdevice

    // reset state for new device
    data->info->type_id = PBIO_IODEV_TYPE_ID_NONE;
    data->iodev.capability_flags = PBIO_IODEV_CAPABILITY_FLAG_NONE;
    data->ext_mode = 0;
    data->status = PBIO_UARTDEV_STATUS_SYNCING;
    // default max tacho rate for BOOST external motor since it is the only
    // motor that does not send this info
    data->max_tacho_rate = 1400;

    pbdrv_uart_flush(data->uart);

    // Send SPEED command at 115200 baud
    PBIO_PT_WAIT_READY(&data->pt, pbdrv_uart_set_baud_rate(data->uart, EV3_UART_SPEED_LPF2));
    debug_pr("set baud: %d\n", EV3_UART_SPEED_LPF2);
    PT_SPAWN(&data->pt, &data->speed_pt, pbio_uartdev_send_speed_msg(data, EV3_UART_SPEED_LPF2));

    // read one byte to check for ACK
    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_begin(data->uart, data->rx_msg, 1, 100));
    if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "UART Rx error during baud");
        goto err;
    }

    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_end(data->uart));
    if ((err == PBIO_SUCCESS && data->rx_msg[0] != LUMP_SYS_ACK) || err == PBIO_ERROR_TIMEDOUT) {
        // if we did not get ACK within 100ms, then switch to slow baud rate for sync
        PBIO_PT_WAIT_READY(&data->pt, pbdrv_uart_set_baud_rate(data->uart, EV3_UART_SPEED_MIN));
        debug_pr("set baud: %d\n", EV3_UART_SPEED_MIN);
    } else if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "UART Rx error during baud");
        goto err;
    }

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

        if (data->rx_msg[0] == (LUMP_MSG_TYPE_CMD | LUMP_CMD_TYPE)) {
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

    if (data->rx_msg[1] < EV3_UART_TYPE_MIN || data->rx_msg[1] > EV3_UART_TYPE_MAX) {
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
    data->data_rec = false;
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
    data->tx_msg[0] = LUMP_SYS_ACK;
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
    debug_pr("set baud: %" PRIu32 "\n", data->new_baud_rate);

    // setting type_id in info struct lets external modules know a device is connected
    data->info->type_id = data->type_id;
    data->status = PBIO_UARTDEV_STATUS_DATA;
    // reset data rx thread
    PT_INIT(&data->data_pt);

    if (PBIO_IODEV_IS_FEEDBACK_MOTOR(&data->iodev)) {
        data->mode_combo_size = __builtin_popcount(data->info->mode_combos) + 2;
        data->mode_combo_payload[0] = 0x20 | (data->mode_combo_size - 2); // mode combo command, x modes
        data->mode_combo_payload[1] = 0; // combo index
        data->mode_combo_payload[2] = 1 << 4 | 0; // mode 1, dataset 0
        data->mode_combo_payload[3] = 2 << 4 | 0; // mode 2, dataset 0
        data->mode_combo_payload[4] = 3 << 4 | 0; // mode 3, dataset 0
        // HACK: we are cheating here and assuming that all mode combinations
        // are consecutive, starting with mode 1 and data->mode_combo_size
        // chops off any unused mode (i.e. APOS)

        // setup motor to send position and speed data
        PBIO_PT_WAIT_READY(&data->pt,
            err = ev3_uart_begin_tx_msg(data, LUMP_MSG_TYPE_CMD, LUMP_CMD_WRITE,
                data->mode_combo_payload, data->mode_combo_size));
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

        // Reset tacho offset
        data->tacho_offset = 0;
    }

    // Turn on power for sensors that need it
    if (PBIO_IODEV_REQUIRES_POWER(&data->iodev)) {
        pbdrv_motor_set_duty_cycle(data->iodev.port, -10000);
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
        data->data_rec = false;

        // send keepalive
        PT_WAIT_WHILE(&data->pt, data->tx_busy);
        data->tx_busy = true;
        data->tx_msg[0] = LUMP_SYS_NACK;
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

    // Turn off power for sensors that used power
    if (PBIO_IODEV_REQUIRES_POWER(&data->iodev)) {
        pbdrv_motor_coast(data->iodev.port);
    }

    process_post(PROCESS_BROADCAST, PROCESS_EVENT_SERVICE_REMOVED, &data->iodev);

    PT_END(&data->pt);
}

// REVISIT: This is not the greatest. We can easily get a buffer overrun and
// loose data. For now, the retry after bad message size helps get back into
// sync with the data stream.
static PT_THREAD(pbio_uartdev_receive_data(uartdev_port_data_t * data)) {
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

        uint8_t msg_type = data->rx_msg[0] & LUMP_MSG_TYPE_MASK;
        uint8_t cmd = data->rx_msg[0] & LUMP_MSG_CMD_MASK;
        if (msg_type != LUMP_MSG_TYPE_DATA && (msg_type != LUMP_MSG_TYPE_CMD ||
                                               (cmd != LUMP_CMD_WRITE && cmd != LUMP_CMD_EXT_MODE))) {
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

    // User mode change for motors is not supported
    if (PBIO_IODEV_IS_FEEDBACK_MOTOR(iodev)) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    err = ev3_uart_begin_tx_msg(port_data, LUMP_MSG_TYPE_CMD, LUMP_CMD_SELECT, &mode, 1);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    port_data->new_mode = mode;
    port_data->mode_change_tx_done = false;

    return PBIO_SUCCESS;
}

static pbio_error_t ev3_uart_set_mode_end(pbio_iodev_t *iodev) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    pbio_error_t err;

    if (!port_data->mode_change_tx_done) {
        err = pbdrv_uart_write_end(port_data->uart);
        if (err != PBIO_ERROR_AGAIN) {
            port_data->tx_busy = false;
            port_data->mode_change_tx_done = true;
        }

        if (err == PBIO_SUCCESS) {
            port_data->data_rec = false;
            return PBIO_ERROR_AGAIN;
        }

        return err;
    }

    if (!port_data->data_rec || port_data->iodev.mode != port_data->new_mode) {
        return PBIO_ERROR_AGAIN;
    }

    port_data->mode_change_tx_done = false;

    return PBIO_SUCCESS;
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

    return ev3_uart_begin_tx_msg(port_data, LUMP_MSG_TYPE_DATA, iodev->mode, data, size);
}

static pbio_error_t ev3_uart_write_begin(pbio_iodev_t *iodev, const uint8_t *data, uint8_t size) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);

    return ev3_uart_begin_tx_msg(port_data, LUMP_MSG_TYPE_CMD, LUMP_CMD_WRITE, data, size);
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
    uartdev_port_data_t *port_data = dev->priv;

    if (!PBIO_IODEV_IS_FEEDBACK_MOTOR(&port_data->iodev)) {
        return PBIO_ERROR_NO_DEV;
    }

    *count = port_data->tacho_count;

    return PBIO_SUCCESS;
}

static pbio_error_t pbio_uartdev_get_abs_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    uartdev_port_data_t *port_data = dev->priv;

    if (!PBIO_IODEV_IS_FEEDBACK_MOTOR(&port_data->iodev)) {
        return PBIO_ERROR_NO_DEV;
    }

    if (!(port_data->iodev.capability_flags & PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS)) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    *count = port_data->abs_pos;

    return PBIO_SUCCESS;
}

static pbio_error_t pbio_uartdev_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    uartdev_port_data_t *port_data = dev->priv;

    if (!PBIO_IODEV_IS_FEEDBACK_MOTOR(&port_data->iodev)) {
        return PBIO_ERROR_NO_DEV;
    }

    // tacho_rate is in percent, so we need to convert it to counts per second
    *rate = port_data->max_tacho_rate * port_data->tacho_rate / 100;

    return PBIO_SUCCESS;
}

static const pbdrv_counter_funcs_t pbio_uartdev_counter_funcs = {
    .get_count = pbio_uartdev_get_count,
    .get_abs_count = pbio_uartdev_get_abs_count,
    .get_rate = pbio_uartdev_get_rate,
};

void pbio_uartdev_counter_init(pbdrv_counter_dev_t *devs) {
    // Since this has async init via contiki process, we need to save this
    // pointer for later.
    counter_devs = devs;
}

static PT_THREAD(pbio_uartdev_init(struct pt *pt, uint8_t id)) {
    const pbio_uartdev_platform_data_t *pdata = &pbio_uartdev_platform_data[id];
    uartdev_port_data_t *port_data = &dev_data[id];

    PT_BEGIN(pt);

    PT_WAIT_UNTIL(pt, pbdrv_uart_get(pdata->uart_id, &port_data->uart) == PBIO_SUCCESS);
    port_data->iodev.info = &infos[id].info;
    port_data->iodev.ops = &pbio_uartdev_ops;
    port_data->info = &infos[id].info;
    port_data->tx_msg = &bufs[id][BUF_TX_MSG][0];
    port_data->rx_msg = &bufs[id][BUF_RX_MSG][0];

    // It is not guaranteed that pbio_uartdev_counter_init() is called before pbio_uartdev_init()
    PT_WAIT_UNTIL(pt, counter_devs != NULL);
    pbdrv_counter_dev_t *counter = &counter_devs[pdata->counter_id];
    counter->funcs = &pbio_uartdev_counter_funcs;
    counter->priv = port_data;

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
            uartdev_port_data_t *data = &dev_data[i];
            pbio_uartdev_update(data);
            if (data->status == PBIO_UARTDEV_STATUS_DATA) {
                pbio_uartdev_receive_data(data);
            }
        }
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_UARTDEV

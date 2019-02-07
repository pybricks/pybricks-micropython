/*
 * Copyright (c) 2018 David Lechner
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Based on:
 * LEGO MINDSTORMS EV3 UART Sensor tty line discipline
 *
 * Copyright (C) 2014-2016,2018 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Relicensed under BSD 3-clause license by author for Pybricks I/O library.
 * This file may be redistributed under either or both licenses.
 */

#define DEBUG 0
#if DEBUG
#define debug_pr(fmt, ...)   printf((fmt), __VA_ARGS__)
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pbdrv/config.h"
#include "pbdrv/ioport.h"
#include "pbdrv/uart.h"
#include "pbio/error.h"
#include "pbio/event.h"
#include "pbio/iodev.h"
#include "pbio/port.h"
#include "pbio/uartdev.h"
#include "sys/etimer.h"
#include "sys/process.h"

#ifndef PBIO_CONFIG_DISABLE_UARTDEV

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
    EV3_UART_INFO_UNK1          = 0x05,    // Powered Up only
    EV3_UART_INFO_UNK2          = 0x06,    // Powered Up only
    EV3_UART_INFO_MODE_PLUS_8   = 0x20,    // Powered Up only - not used?
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
    EV3_UART_INFO_BIT_INFO_UNK1,
    EV3_UART_INFO_BIT_INFO_UNK2,
    EV3_UART_INFO_BIT_INFO_FORMAT,
};

enum ev3_uart_info_flags {
    EV3_UART_INFO_FLAG_CMD_TYPE     = 1 << EV3_UART_INFO_BIT_CMD_TYPE,
    EV3_UART_INFO_FLAG_CMD_MODES    = 1 << EV3_UART_INFO_BIT_CMD_MODES,
    EV3_UART_INFO_FLAG_CMD_SPEED    = 1 << EV3_UART_INFO_BIT_CMD_SPEED,
    EV3_UART_INFO_FLAG_CMD_VERSION  = 1 << EV3_UART_INFO_BIT_CMD_VERSION,
    EV3_UART_INFO_FLAG_INFO_NAME    = 1 << EV3_UART_INFO_BIT_INFO_NAME,
    EV3_UART_INFO_FLAG_INFO_RAW     = 1 << EV3_UART_INFO_BIT_INFO_RAW,
    EV3_UART_INFO_FLAG_INFO_PCT     = 1 << EV3_UART_INFO_BIT_INFO_PCT,
    EV3_UART_INFO_FLAG_INFO_SI      = 1 << EV3_UART_INFO_BIT_INFO_SI,
    EV3_UART_INFO_FLAG_INFO_UNITS   = 1 << EV3_UART_INFO_BIT_INFO_UNITS,
    EV3_UART_INFO_FLAG_INFO_UNK1    = 1 << EV3_UART_INFO_BIT_INFO_UNK1,
    EV3_UART_INFO_FLAG_INFO_UNK2    = 1 << EV3_UART_INFO_BIT_INFO_UNK2,
    EV3_UART_INFO_FLAG_INFO_FORMAT  = 1 << EV3_UART_INFO_BIT_INFO_FORMAT,
    EV3_UART_INFO_FLAG_ALL_INFO     = EV3_UART_INFO_FLAG_INFO_NAME
                                    | EV3_UART_INFO_FLAG_INFO_RAW
                                    | EV3_UART_INFO_FLAG_INFO_PCT
                                    | EV3_UART_INFO_FLAG_INFO_SI
                                    | EV3_UART_INFO_FLAG_INFO_UNITS
                                    | EV3_UART_INFO_FLAG_INFO_UNK1
                                    | EV3_UART_INFO_FLAG_INFO_UNK2
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
 * struct ev3_uart_data - Discipline data for EV3 UART Sensor communication
 * @iodev: Pointer to the I/O device
 * @timer: Timer for sending keepalive messages.
 * @requested_mode: Mode that was requested by user. Used to restore previous
 * 	mode in case of a reconnect.
 * @new_mode: The mode requested by set_mode. Also used to keep track of mode
 *  in INFO messages while syncing.
 * @new_baud_rate: New baud rate that will be set with ev3_uart_change_bitrate
 * @info_flags: Flags indicating what information has already been read
 * 	from the data.
 * @msg: partial data->msg from previous receive callback
 * @partial_msg_size: the size of the partial data->msg
 * @ext_mode: Extra mode adder for Powered Up devices (for modes > EV3_UART_MODE_MAX)
 * @write_cmd_size: The size parameter received from a WRITE command
 * @last_err: data->msg to be printed in case of an error.
 * @num_data_err: Number of bad reads when receiving DATA data->msgs.
 * @data_rec: Flag that indicates that good DATA data->msg has been received
 * 	since last watchdog timeout.
 */
typedef struct ev3_uart_port_data {
    pbio_iodev_t *iodev;
    struct etimer timer;
    pbio_uartdev_status_t status;
    uint8_t requested_mode;
    uint8_t new_mode;
    uint32_t new_baud_rate;
    uint32_t info_flags;
    uint8_t msg[EV3_UART_MAX_MESSAGE_SIZE];
    uint8_t partial_msg_size;
    uint8_t ext_mode;
    uint8_t write_cmd_size;
    DBG_ERR(const char *last_err);
    uint32_t num_data_err;
    unsigned data_rec:1;
} uartdev_port_data_t;

PROCESS(pbio_uartdev_process, "UART device");

static uartdev_port_data_t dev_data[PBDRV_CONFIG_NUM_IO_PORT];

static const pbio_iodev_mode_t ev3_uart_default_mode_info = {
    .raw_max    = 1023,
    .pct_max    = 100,
    .si_max     = 1,
    .digits     = 4,
};

static inline uint8_t port_to_index(pbio_port_t port) {
    return port - PBDRV_CONFIG_FIRST_IO_PORT;
}

static inline bool test_and_set_bit(uint8_t bit, uint32_t *flags) {
    bool result = *flags & (1 << bit);
    *flags |= (1 << bit);
    return result;
}

static uint8_t ev3_uart_get_msg_size(uint8_t header) {
    uint8_t size;

    if (!(header & EV3_UART_MSG_TYPE_MASK)) {
        // this is a SYS message (i.e. SYNC, NACK, ACK)
        return 1;
    }

    size = EV3_UART_CMD_SIZE(header);
    size += 2; /* header and checksum */
    if ((header & EV3_UART_MSG_TYPE_MASK) == EV3_UART_MSG_TYPE_INFO)
        size++; /* extra command byte */

    return size;
}

static void pbio_uartdev_put(pbio_port_t port, uint8_t next_byte) {
    uartdev_port_data_t *data;
    uint32_t speed;
    uint8_t msg_size, msg_type, cmd, cmd2, mode, i;

    data = &dev_data[port_to_index(port)];

    if (data->status == PBIO_UARTDEV_STATUS_ERR) {
        data->partial_msg_size = 0;
        data->ext_mode = 0;
        data->status = PBIO_UARTDEV_STATUS_SYNCING;
    }

    /*
     * To get in sync with the data stream from the sensor, we look
     * for a valid TYPE command.
     */
    if (data->status == PBIO_UARTDEV_STATUS_SYNCING) {
        // look for first byte that is TYPE command
        if (data->partial_msg_size == 0) {
            if (next_byte == (EV3_UART_MSG_TYPE_CMD | EV3_UART_CMD_TYPE)) {
                data->msg[0] = next_byte;
                data->partial_msg_size = 1;
            }
            return;
        }

        // look for second byte that is valid device type ID
        if (data->partial_msg_size == 1) {
            if (next_byte <= EV3_UART_TYPE_MIN || next_byte <= EV3_UART_TYPE_MAX) {
                data->msg[1] = next_byte;
                data->partial_msg_size = 2;
            }
            else {
                data->partial_msg_size = 0;
            }
            return;
        }

        // look for third byte that is valid checksum
        if (data->partial_msg_size == 2) {
            uint8_t checksum = 0xff ^ data->msg[0] ^ data->msg[1];

            if (next_byte != checksum) {
                data->partial_msg_size = 0;
                return;
            }
        }

        data->iodev->info->num_modes = 1;
        data->iodev->info->num_view_modes = 1;

        for (i = 0; i < PBIO_IODEV_MAX_NUM_MODES; i++) {
            data->iodev->info->mode_info[i] = ev3_uart_default_mode_info;
        }

        data->iodev->info->type_id = data->msg[1];
        data->partial_msg_size = 0;
        data->info_flags = EV3_UART_INFO_FLAG_CMD_TYPE;
        data->data_rec = 0;
        data->num_data_err = 0;
        data->status = PBIO_UARTDEV_STATUS_INFO;

        return;
    }

    if (data->partial_msg_size) {
        // collect next_byte until we have a full data->msg
        msg_size = ev3_uart_get_msg_size(data->msg[0]);
        data->msg[data->partial_msg_size++] = next_byte;
        if (data->partial_msg_size < msg_size) {
            return;
        }
    } else if (next_byte == 0xFF) {
        // Sometimes we get 0xFF after switching baud rates, so just ignore it.
        return;
    } else {
        // first byte of the data->msg contains the data->msg size
        msg_size = ev3_uart_get_msg_size(next_byte);
        if (msg_size > EV3_UART_MAX_MESSAGE_SIZE) {
            DBG_ERR(data->last_err = "Bad data->msg size");
            goto err;
        }
        data->msg[0] = next_byte;
        data->partial_msg_size = 1;
        if (msg_size > 1) {
            return;
        }
    }

    // at this point, we have a full data->msg that can be parsed

    // reset msg size for next message
    data->partial_msg_size = 0;

    msg_type = data->msg[0] & EV3_UART_MSG_TYPE_MASK;
    cmd = data->msg[0] & EV3_UART_MSG_CMD_MASK;
    mode = cmd;
    cmd2 = data->msg[1];

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
        for (i = 0; i < msg_size - 1; i++) {
            checksum ^= data->msg[i];
        }
        /*
         * The LEGO EV3 color sensor sends bad checksums
         * for RGB-RAW data (mode 4). The check here could be
         * improved if someone can find a pattern.
         */
        if (checksum != data->msg[msg_size - 1]
            && data->iodev->info->type_id != PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR
            && data->msg[0] != 0xDC)
        {
            DBG_ERR(data->last_err = "Bad checksum");
            // if INFO messages are done and we are now receiving data, it is
            // OK to occasionally have a bad checksum
            if (data->status == PBIO_UARTDEV_STATUS_DATA) {
                data->num_data_err++;
                return;
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
            if (!data->iodev->info->num_modes) {
                DBG_ERR(data->last_err = "Received ACK before all mode INFO");
                goto err;
            }
            if ((data->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                DBG_ERR(data->last_err = "Did not receive all required INFO");
                goto err;
            }

            data->status = PBIO_UARTDEV_STATUS_ACK;
            data->iodev->mode = data->new_mode;

            // reply with ACK
            while (pbdrv_uart_put_char(port, EV3_UART_SYS_ACK) == PBIO_ERROR_AGAIN);

            // schedule baud rate change
            etimer_set(&data->timer, clock_from_msec(10));

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
            data->iodev->info->num_modes = cmd2 + 1;
            if (msg_size > 5) {
                // Powered Up devices can have an extended mode message that
                // includes modes > EV3_UART_MODE_MAX
                data->iodev->info->num_modes = data->msg[3] + 1;
                data->iodev->info->num_view_modes = data->msg[4] + 1;
            }
            else if (msg_size > 3) {
                data->iodev->info->num_view_modes = data->msg[2] + 1;
            }
            else {
                data->iodev->info->num_view_modes = data->iodev->info->num_modes;
            }

            debug_pr("num_modes: %d\n", data->iodev->info->num_modes);
            debug_pr("num_view_modes: %d\n", data->iodev->info->num_view_modes);

            break;
        case EV3_UART_CMD_SPEED:
            if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_SPEED, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate speed INFO");
                goto err;
            }
            speed = *(int*)(data->msg + 1);
            if (speed < EV3_UART_SPEED_MIN || speed > EV3_UART_SPEED_MAX) {
                DBG_ERR(data->last_err = "Speed is out of range");
                goto err;
            }
            data->new_baud_rate = speed;

            debug_pr("speed: %lu\n", speed);

            break;
        case EV3_UART_CMD_WRITE:
            if (cmd2 & 0x20) {
                data->write_cmd_size = cmd2 & 0x3;
                if (data->iodev->info->type_id == PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR) {
                    // TODO: msg[3] and msg[4] probably give us useful information
                    data->iodev->flags |= PBIO_IODEV_FLAG_IS_MOTOR;
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
            data->ext_mode = data->msg[1];
            break;
        case EV3_UART_CMD_VERSION:
            if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_VERSION, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate version INFO");
                goto err;
            }
            // TODO: this might be useful someday

            debug_pr("version: %08lx\n", *(uint32_t *)(data->msg + 1));

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
            if (data->msg[2] < 'A' || data->msg[2] > 'z') {
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
            data->msg[msg_size - 1] = 0;
            if (strlen((char *)data->msg + 2) > PBIO_IODEV_MODE_NAME_SIZE) {
                DBG_ERR(data->last_err = "Name is too long");
                goto err;
            }
            snprintf(data->iodev->info->mode_info[mode].name,
                        PBIO_IODEV_MODE_NAME_SIZE + 1, "%s",
                        data->msg + 2);
            data->new_mode = mode;
            data->info_flags |= EV3_UART_INFO_FLAG_INFO_NAME;

            debug_pr("name: %s\n", data->iodev->info->mode_info[mode].name);

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
            data->iodev->info->mode_info[mode].raw_min = *(float *)(data->msg + 2);
            data->iodev->info->mode_info[mode].raw_max = *(float *)(data->msg + 6);

            debug_pr("raw: %f %f\n", data->iodev->info->mode_info[mode].raw_min,
                                     data->iodev->info->mode_info[mode].raw_max);

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
            data->iodev->info->mode_info[mode].pct_min = *(float *)(data->msg + 2);
            data->iodev->info->mode_info[mode].pct_max = *(float *)(data->msg + 6);

            debug_pr("pct: %f %f\n", data->iodev->info->mode_info[mode].pct_min,
                                     data->iodev->info->mode_info[mode].pct_max);

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
            data->iodev->info->mode_info[mode].si_min = *(float *)(data->msg + 2);
            data->iodev->info->mode_info[mode].si_max = *(float *)(data->msg + 6);

            debug_pr("si: %f %f\n", data->iodev->info->mode_info[mode].si_min,
                                    data->iodev->info->mode_info[mode].si_max);

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
            data->msg[msg_size - 1] = 0;
            snprintf(data->iodev->info->mode_info[mode].uom, PBIO_IODEV_UOM_SIZE + 1,
                     "%s", data->msg + 2);

            debug_pr("uom: %s\n", data->iodev->info->mode_info[mode].uom);

            break;
        case EV3_UART_INFO_UNK1:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK1, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate UNK1 units INFO");
                goto err;
            }
            // TODO: what does this info tell us?

            debug_pr("UNK1: %02x %02x\n", data->msg[2], data->msg[3]);

            break;
        case EV3_UART_INFO_UNK2:
            if (data->new_mode != mode) {
                DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                goto err;
            }
            if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK2, &data->info_flags)) {
                DBG_ERR(data->last_err = "Received duplicate UNK2 units INFO");
                goto err;
            }
            // TODO: what does this info tell us?

            debug_pr("UNK2: %02x %02x\n", data->msg[2], data->msg[3]);

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
            data->iodev->info->mode_info[mode].num_values = data->msg[2];
            if (!data->iodev->info->mode_info[mode].num_values) {
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
            switch (data->msg[3]) {
            case EV3_UART_DATA_8:
                data->iodev->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_INT8;
                break;
            case EV3_UART_DATA_16:
                data->iodev->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_INT16;
                break;
            case EV3_UART_DATA_32:
                data->iodev->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_INT32;
                break;
            case EV3_UART_DATA_FLOAT:
                data->iodev->info->mode_info[mode].data_type = PBIO_IODEV_DATA_TYPE_FLOAT;
                break;
            default:
                DBG_ERR(data->last_err = "Invalid data type");
                goto err;
            }
            data->iodev->info->mode_info[mode].digits = data->msg[4];
            data->iodev->info->mode_info[mode].decimals = data->msg[5];
            if (data->new_mode) {
                data->new_mode--;
            }

            debug_pr("num_values: %d\n", data->iodev->info->mode_info[mode].num_values);
            debug_pr("data_type: %d\n", data->iodev->info->mode_info[mode].data_type);
            debug_pr("digits: %d\n", data->iodev->info->mode_info[mode].digits);
            debug_pr("decimals: %d\n", data->iodev->info->mode_info[mode].decimals);

            break;
        }
        break;
    case EV3_UART_MSG_TYPE_DATA:
        if (data->status != PBIO_UARTDEV_STATUS_DATA) {
            DBG_ERR(data->last_err = "Received DATA before INFO was complete");
            goto err;
        }

        if (data->iodev->info->type_id == PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR && data->write_cmd_size > 0) {
            memcpy(data->iodev->bin_data, data->msg + 1, 6);
        }
        else {
            if (mode >= data->iodev->info->num_modes) {
                DBG_ERR(data->last_err = "Invalid mode received");
                goto err;
            }
            if (mode != data->iodev->mode) {
                if (mode == data->new_mode) {
                    data->iodev->mode = mode;
                    // TODO: notify that mode has changed
                } else {
                    DBG_ERR(data->last_err = "Unexpected mode");
                    goto err;
                }
            }
            memcpy(data->iodev->bin_data, data->msg + 1, msg_size - 2);
        }

        data->data_rec = 1;
        if (data->num_data_err) {
            data->num_data_err--;
        }
        break;
    }

    return;

err:
    data->status = PBIO_UARTDEV_STATUS_ERR;
    data->new_baud_rate = EV3_UART_SPEED_MIN;
    etimer_stop(&data->timer);
    pbdrv_uart_set_baud_rate(port, EV3_UART_SPEED_MIN);
    debug_pr("%s\n", data->last_err);
}

static uint8_t ev3_uart_set_msg_hdr(enum ev3_uart_msg_type type, enum ev3_uart_msg_size size, enum ev3_uart_cmd cmd)
{
    return (type & EV3_UART_MSG_TYPE_MASK) | (size & EV3_UART_MSG_SIZE_MASK) | (cmd & EV3_UART_MSG_CMD_MASK);
}

static pbio_error_t _ev3_uart_write(pbio_port_t port, enum ev3_uart_msg_type msg_type, enum ev3_uart_cmd cmd, const uint8_t *data, uint8_t len) {
    uint8_t msg[EV3_UART_MAX_MESSAGE_SIZE - 2];
    uint8_t header, checksum, i;
    enum ev3_uart_msg_size size;

    checksum = 0xff;
    for (i = 0; i < len; i++) {
        msg[i] = data[i];
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
        msg[i] = 0;
    }

    header = ev3_uart_set_msg_hdr(msg_type, size, cmd);
    checksum ^= header;

    while (pbdrv_uart_put_char(port, header) == PBIO_ERROR_AGAIN);
    for (i = 0; i < len; i++) {
        while (pbdrv_uart_put_char(port, msg[i]) == PBIO_ERROR_AGAIN);
    }
    while (pbdrv_uart_put_char(port, checksum) == PBIO_ERROR_AGAIN);

    return PBIO_SUCCESS;
}

static pbio_error_t ev3_uart_set_data(pbio_iodev_t *iodev, uint8_t *data) {
    uartdev_port_data_t *port_data = &dev_data[port_to_index(iodev->port)];
    uint8_t header, ext_mode, checksum, size;

    if (port_data->status != PBIO_UARTDEV_STATUS_DATA) {
        return PBIO_ERROR_AGAIN;
    }

    // TODO: not all modes support setting data. Need to find a way to check
    // this and return PBIO_ERROR_INVALID_OP.

    // Only Powered Up devices support setting data, and they expect to have an
    // extra command sent to give the part of the mode > 7
    header = ev3_uart_set_msg_hdr(EV3_UART_MSG_TYPE_CMD, EV3_UART_MSG_SIZE_1, EV3_UART_CMD_EXT_MODE);
    ext_mode = iodev->mode > EV3_UART_MODE_MAX ? 8 : 0;
    checksum = 0xff ^ header ^ ext_mode;
    while (pbdrv_uart_put_char(iodev->port, header) == PBIO_ERROR_AGAIN);
    while (pbdrv_uart_put_char(iodev->port, ext_mode) == PBIO_ERROR_AGAIN);
    while (pbdrv_uart_put_char(iodev->port, checksum) == PBIO_ERROR_AGAIN);

    size = iodev->info->mode_info[iodev->mode].num_values * pbio_iodev_size_of(iodev->info->mode_info[iodev->mode].data_type);

    return _ev3_uart_write(iodev-> port, EV3_UART_MSG_TYPE_DATA, iodev->mode, data, size);
}

static pbio_error_t ev3_uart_write(pbio_iodev_t *iodev, const uint8_t *data, uint8_t len) {
    uartdev_port_data_t *port_data = &dev_data[port_to_index(iodev->port)];

    if (port_data->status != PBIO_UARTDEV_STATUS_DATA) {
        return PBIO_ERROR_AGAIN;
    }

    if (len == 0 || len >= EV3_UART_MAX_MESSAGE_SIZE - 2) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return _ev3_uart_write(iodev-> port, EV3_UART_MSG_TYPE_CMD, EV3_UART_CMD_WRITE, data, len);
}

static pbio_error_t ev3_uart_set_mode(pbio_iodev_t *iodev, uint8_t mode) {
    uartdev_port_data_t *port_data = &dev_data[port_to_index(iodev->port)];
    uint8_t header, checksum;

    if (port_data->status != PBIO_UARTDEV_STATUS_DATA) {
        return PBIO_ERROR_AGAIN;
    }

    port_data->new_mode = mode;
    header = ev3_uart_set_msg_hdr(EV3_UART_MSG_TYPE_CMD, EV3_UART_MSG_SIZE_1, EV3_UART_CMD_SELECT);
    checksum = 0xff ^ header ^ mode;

    while (pbdrv_uart_put_char(iodev->port, header) == PBIO_ERROR_AGAIN);
    while (pbdrv_uart_put_char(iodev->port, mode) == PBIO_ERROR_AGAIN);
    while (pbdrv_uart_put_char(iodev->port, checksum) == PBIO_ERROR_AGAIN);

    return PBIO_SUCCESS;
}

PROCESS_THREAD(pbio_uartdev_process, ev, data) {

    PROCESS_BEGIN();

    for (int i = 0; i < PBDRV_CONFIG_NUM_IO_PORT; i++) {
        pbdrv_ioport_get_iodev(PBDRV_CONFIG_FIRST_IO_PORT + i, &dev_data[i].iodev);
    }

    while (true) {
        PROCESS_WAIT_EVENT();
        if (ev == PBIO_EVENT_UART_RX) {
            pbio_event_uart_rx_data_t rx = { .data = data };
            pbio_uartdev_put(rx.port, rx.byte);
        }
        else if (ev == PROCESS_EVENT_TIMER) {
            for (int i = 0; i < PBDRV_CONFIG_NUM_IO_PORT; i++) {
                // keepalive timer
                if (etimer_expired(&dev_data[i].timer)) {
                    if (dev_data[i].status == PBIO_UARTDEV_STATUS_ACK) {
                        // change the baud rate
                        etimer_reset_with_new_interval(&dev_data[i].timer, clock_from_msec(EV3_UART_DATA_KEEP_ALIVE_TIMEOUT));
                        pbdrv_uart_set_baud_rate(dev_data[i].iodev->port, dev_data[i].new_baud_rate);
                        dev_data[i].status = PBIO_UARTDEV_STATUS_DATA;
                        dev_data[i].iodev->set_data = ev3_uart_set_data;
                        dev_data[i].iodev->write = ev3_uart_write;
                        dev_data[i].iodev->set_mode = ev3_uart_set_mode;

                        if (dev_data[i].iodev->info->type_id == PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR) {
                            const uint8_t msg[] = { 0x22, 0x00, 0x10, 0x20 };

                            // send magic sequence to tell motor to send position and speed data
                            while (ev3_uart_write(dev_data[i].iodev, msg, 4) == PBIO_ERROR_AGAIN);
                        }
                    }
                    else if (dev_data[i].num_data_err > 6) {
                        etimer_stop(&dev_data[i].timer);
                        dev_data[i].status = PBIO_UARTDEV_STATUS_ERR;
                        pbdrv_uart_set_baud_rate(dev_data[i].iodev->port, EV3_UART_SPEED_MIN);
                    }
                    else {
                        etimer_reset(&dev_data[i].timer);
                        if (!dev_data[i].data_rec) {
                            dev_data[i].num_data_err++;
                            DBG_ERR(dev_data[i].last_err = "No data since last keepalive");
                        }
                        // send keepalive
                        while (pbdrv_uart_put_char(dev_data[i].iodev->port, EV3_UART_SYS_NACK) == PBIO_ERROR_AGAIN);
                        dev_data[i].data_rec = 0;
                    }
                }
            }
        }
    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_UARTDEV

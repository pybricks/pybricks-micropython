// SPDX-License-Identifier: MIT OR GPL-2.0-only
// Copyright (c) 2018-2023 The Pybricks Authors

/*
 * Based on:
 * LEGO MINDSTORMS EV3 UART Sensor tty line discipline
 *
 * Copyright (c) 2014-2016,2018-2019 David Lechner <david@lechnology.com>
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

// Enables processing of information not needed for normal operation
#define UARTDEV_ENABLE_EXTRAS (DEBUG)

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
#include <pbdrv/motor_driver.h>

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
    EV3_UART_INFO_BIT_INFO_UNK11,
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
    /** Something bad happened. */
    PBIO_UARTDEV_STATUS_ERR,
    /** Waiting for port to be placed in UART mode with device attached. */
    PBIO_UARTDEV_STATUS_WAITING,
    /**< Waiting for data that looks like LEGO UART protocol. */
    PBIO_UARTDEV_STATUS_SYNCING,
    /**< Reading device info before changing baud rate. */
    PBIO_UARTDEV_STATUS_INFO,
    /**< ACK received, delay changing baud rate. */
    PBIO_UARTDEV_STATUS_ACK,
    /**< Ready to send commands and receive data. */
    PBIO_UARTDEV_STATUS_DATA,
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
 * @new_mode: The mode requested by set_mode. Also used to keep track of mode
 *  in INFO messages while syncing.
 * @new_baud_rate: New baud rate that will be set with ev3_uart_change_bitrate
 * @info_flags: Flags indicating what information has already been read
 *      from the data.
 * @abs_pos: The absolute position received from an LPF2 motor
 * @tx_msg: Buffer to hold messages transmitted to the device
 * @rx_msg: Buffer to hold messages received from the device
 * @rx_msg_size: Size of the current message being received
 * @ext_mode: Extra mode adder for Powered Up devices (for modes > LUMP_MAX_MODE)
 * @write_cmd_size: The size parameter received from a WRITE command
 * @last_err: data->msg to be printed in case of an error.
 * @err_count: Total number of errors that have occurred
 * @num_data_err: Number of bad reads when receiving DATA data->msgs.
 * @mode_switch_time: Time of most recent successful mode switch, used to discard stale data.
 * @tx_start_time: Time of most recently started transmission.
 * @data_rec: Flag that indicates that good DATA data->msg has been received
 *      since last watchdog timeout.
 * @tx_busy: mutex that protects tx_msg
 * @tx_type: The data type of the current or last transmission.
 * @data_set_len: Length of data to be set, data is stored in bin_data.
 * @speed_payload: Buffer for holding baud rate change message data
 */
typedef struct {
    pbio_iodev_t iodev;
    struct pt pt;
    struct pt data_pt;
    struct pt speed_pt;
    struct etimer timer;
    pbdrv_uart_dev_t *uart;
    pbio_iodev_info_t *info;
    pbdrv_motor_driver_dev_t *motor_driver;
    pbio_uartdev_status_t status;
    pbio_iodev_type_id_t type_id;
    uint8_t new_mode;
    uint32_t new_baud_rate;
    uint32_t info_flags;
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t rx_msg_size;
    uint8_t ext_mode;
    uint8_t write_cmd_size;
    DBG_ERR(const char *last_err);
    uint32_t err_count;
    uint32_t num_data_err;
    uint32_t mode_switch_time;
    uint32_t tx_start_time;
    bool data_rec;
    bool tx_busy;
    uint8_t data_set_len;
    lump_msg_type_t tx_type;
    uint8_t speed_payload[4];
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

#define PBIO_PT_WAIT_READY(pt, expr) PT_WAIT_UNTIL((pt), (expr) != PBIO_ERROR_AGAIN)

pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev) {
    if (id >= PBIO_CONFIG_UARTDEV_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *iodev = &dev_data[id].iodev;

    if (!(*iodev)->info) {
        // device has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Indicates to the uartdev driver that the port has been placed in uart mode
 * (i.e. pin mux) and is ready to start syncing with the attached I/O device.
 * @param [in] id The uartdev device id.
 */
void pbio_uartdev_ready(uint8_t id) {
    if (id >= PBIO_CONFIG_UARTDEV_NUM_DEV) {
        return;
    }

    uartdev_port_data_t *data = &dev_data[id];

    // REVISIT: For now we are assuming that this function is never called
    // at the wrong time. If this assumption turns out to be false, we will
    // need to return PBIO_ERROR_AGAIN and modify callers to retry.
    if (data->status != PBIO_UARTDEV_STATUS_WAITING) {
        return;
    }

    // notify pbio_uartdev_update() that there should be a device ready to
    // communicate with now
    data->status = PBIO_UARTDEV_STATUS_SYNCING;
    process_poll(&pbio_uartdev_process);
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
                    }

                    debug_pr("num_modes: %d\n", data->info->num_modes);

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
                    #if UARTDEV_ENABLE_EXTRAS
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_VERSION, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate version INFO");
                        goto err;
                    }
                    // TODO: this might be useful someday
                    debug_pr("fw version: %08" PRIx32 "\n", pbio_get_uint32_le(data->rx_msg + 1));
                    debug_pr("hw version: %08" PRIx32 "\n", pbio_get_uint32_le(data->rx_msg + 5));
                    #endif // LUMP_CMD_VERSION
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
                    data->new_mode = mode;
                    data->info_flags |= EV3_UART_INFO_FLAG_INFO_NAME;

                    // newer LEGO UART devices send additional 6 mode capability flags
                    uint8_t flags = 0;
                    if (name_len <= LUMP_MAX_SHORT_NAME_SIZE && msg_size > LUMP_MAX_NAME_SIZE) {
                        // Only the first is used in practice.
                        flags = data->rx_msg[8];
                    } else {
                        // for newer devices that don't send it, set flags by device ID
                        // TODO: Look up from static info like we do for basic devices
                        if (data->type_id == PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR) {
                            flags = LUMP_MODE_FLAGS0_MOTOR | LUMP_MODE_FLAGS0_MOTOR_POWER | LUMP_MODE_FLAGS0_MOTOR_SPEED | LUMP_MODE_FLAGS0_MOTOR_REL_POS;
                        }
                    }

                    // Although capabilities are sent per mode, we apply them to the whole device
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_POWER) {
                        data->info->capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_SPEED) {
                        data->info->capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_SPEED;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_REL_POS) {
                        data->info->capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_ABS_POS) {
                        data->info->capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS;
                    }
                    if (flags & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN1) {
                        data->info->capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1;
                    }
                    if (flags & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN2) {
                        data->info->capability_flags |= PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2;
                    }

                    debug_pr("new_mode: %d\n", data->new_mode);
                    debug_pr("flags: %02X %02X %02X %02X %02X %02X\n",
                        data->rx_msg[8 + 0], data->rx_msg[8 + 1], data->rx_msg[8 + 2],
                        data->rx_msg[8 + 3], data->rx_msg[8 + 4], data->rx_msg[8 + 5]);
                }
                break;
                #if UARTDEV_ENABLE_EXTRAS
                case LUMP_INFO_RAW:
                case LUMP_INFO_PCT:
                case LUMP_INFO_SI:
                case LUMP_INFO_UNITS:
                    break;
                #endif // UARTDEV_ENABLE_EXTRAS
                case LUMP_INFO_MAPPING:
                    if (data->new_mode != mode) {
                        DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MAPPING, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate mapping INFO");
                        goto err;
                    }

                    // Mode supports writing if rx_msg[3] is nonzero. Store in 3rd bit of data type.
                    data->info->mode_info[mode].data_type = (data->rx_msg[3] != 0) << 2;

                    debug_pr("mapping: in %02x out %02x\n", data->rx_msg[2], data->rx_msg[3]);
                    debug_pr("mapping: in %02x out %02x\n", data->rx_msg[2], data->rx_msg[3]);
                    debug_pr("Writable: %d\n", data->info->mode_info[mode].data_type & PBIO_IODEV_DATA_TYPE_WRITABLE);

                    break;
                #if UARTDEV_ENABLE_EXTRAS
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
                    debug_pr("mode combos: %04x\n", data->rx_msg[3] << 8 | data->rx_msg[2]);

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

                    // first 3 parameters look like PID constants, 4th is max tacho_rate
                    debug_pr("motor parameters: %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
                        pbio_get_uint32_le(data->rx_msg + 2), pbio_get_uint32_le(data->rx_msg + 6),
                        pbio_get_uint32_le(data->rx_msg + 10), pbio_get_uint32_le(data->rx_msg + 14));

                    break;
                case LUMP_INFO_UNK11:
                    if (data->new_mode != mode) {
                        DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK11, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate UNK11 INFO");
                        goto err;
                    }
                    break;
                #endif // UARTDEV_ENABLE_EXTRAS
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
                    // Mode writability has been set previously. Now OR with mode type.
                    data->info->mode_info[mode].data_type |= data->rx_msg[3];
                    if (data->new_mode) {
                        data->new_mode--;
                    }

                    debug_pr("num_values: %d\n", data->info->mode_info[mode].num_values);
                    debug_pr("data_type: %d\n", data->info->mode_info[mode].data_type & PBIO_IODEV_DATA_TYPE_MASK);

                    break;
            }
            break;
        case LUMP_MSG_TYPE_DATA:
            if (data->status != PBIO_UARTDEV_STATUS_DATA) {
                DBG_ERR(data->last_err = "Received DATA before INFO was complete");
                goto err;
            }

            if (mode >= data->info->num_modes) {
                DBG_ERR(data->last_err = "Invalid mode received");
                goto err;
            }

            // Data is for requested mode.
            if (mode == data->new_mode) {
                if (!data->data_set_len) {
                    // Copy the new data unless buffer contains data for sending.
                    memcpy(data->iodev.bin_data, data->rx_msg + 1, msg_size - 2);
                }

                if (data->iodev.mode != mode) {
                    // First time getting data in this mode, so register time.
                    data->mode_switch_time = pbdrv_clock_get_ms();
                }
            }
            data->iodev.mode = mode;

            // setting type_id in info struct lets external modules know a device is connected and receiving good data
            data->info->type_id = data->type_id;

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

    if (port_data->tx_busy) {
        return PBIO_ERROR_AGAIN;
    }

    port_data->tx_busy = true;
    port_data->tx_type = msg_type;
    port_data->tx_start_time = pbdrv_clock_get_ms();

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

    // FIXME: The pin1/pin2 power control should be implemented via a callback
    // to the port that the I/O device is attached to instead of poking the
    // motor driver directly. The current implementation is only valid on
    // Powered Up platforms and it assumes that motor driver id corresponds to
    // the port.

    #ifdef PBDRV_CONFIG_FIRST_MOTOR_PORT
    if (pbdrv_motor_driver_get_dev(data->iodev.port - PBDRV_CONFIG_FIRST_MOTOR_PORT, &data->motor_driver) != PBIO_SUCCESS) {
        data->motor_driver = NULL;
    }
    #endif

    // reset state for new device
    data->info->type_id = PBIO_IODEV_TYPE_ID_NONE;
    data->info->capability_flags = PBIO_IODEV_CAPABILITY_FLAG_NONE;
    data->ext_mode = 0;
    data->data_set_len = 0;
    data->status = PBIO_UARTDEV_STATUS_WAITING;

    // block until pbio_uartdev_ready() is called
    PT_WAIT_UNTIL(&data->pt, data->status == PBIO_UARTDEV_STATUS_SYNCING);

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
    etimer_set(&data->timer, 10);
    PT_WAIT_UNTIL(&data->pt, etimer_expired(&data->timer));

    // change the baud rate
    PBIO_PT_WAIT_READY(&data->pt, pbdrv_uart_set_baud_rate(data->uart, data->new_baud_rate));
    debug_pr("set baud: %" PRIu32 "\n", data->new_baud_rate);

    data->status = PBIO_UARTDEV_STATUS_DATA;
    // reset data rx thread
    PT_INIT(&data->data_pt);

    // Turn on power for devices that need it
    if (data->motor_driver) {
        if (data->info->capability_flags & PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1) {
            pbdrv_motor_driver_set_duty_cycle(data->motor_driver, -PBDRV_MOTOR_DRIVER_MAX_DUTY);
        } else if (data->info->capability_flags & PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2) {
            pbdrv_motor_driver_set_duty_cycle(data->motor_driver, PBDRV_MOTOR_DRIVER_MAX_DUTY);
        } else {
            pbdrv_motor_driver_coast(data->motor_driver);
        }
    }

    while (data->status == PBIO_UARTDEV_STATUS_DATA) {
        // setup keepalive timer
        etimer_reset_with_new_interval(&data->timer, EV3_UART_DATA_KEEP_ALIVE_TIMEOUT);
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

    // Turn off battery supply to this port
    if (data->motor_driver) {
        pbdrv_motor_driver_coast(data->motor_driver);
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

/**
 * Gets the minimum time needed before stale data is discarded.
 *
 * This is empirically determined based on sensor experiments.
 *
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
static uint32_t pbio_iodev_delay_stale_data(pbio_iodev_type_id_t id, uint8_t mode) {
    switch (id) {
        case PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR:
            return mode == PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX ? 0 : 30;
        case PBIO_IODEV_TYPE_ID_SPIKE_COLOR_SENSOR:
            return mode == PBIO_IODEV_MODE_PUP_COLOR_SENSOR__LIGHT ? 0 : 30;
        case PBIO_IODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR:
            return mode == PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__LIGHT ? 0 : 50;
        default:
            // Default delay for other sensors and modes.
            return 0;
    }
}

/**
 * Gets the minimum time needed for the device to handle written data.
 *
 * This is empirically determined based on sensor experiments.
 *
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
static uint32_t pbio_iodev_delay_set_data(pbio_iodev_type_id_t id, uint8_t mode) {
    // The Boost Color Distance Sensor requires a long delay or successive
    // writes are ignored.
    if (id == PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR && mode == PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX) {
        return 250;
    }

    // Default delay for setting data. In practice, this is the delay for setting
    // the light on the color sensor and ultrasonic sensor.
    return 2;
}

/**
 * Checks if LEGO UART device mode change or data set operation is complete.
 *
 * @param [in]  iodev       The I/O device
 * @return                  @c true if ready, @c false otherwise.
 */
static bool pbio_uartdev_operation_complete(pbio_iodev_t *iodev) {

    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    const pbio_iodev_type_id_t id = iodev->info->type_id;
    const uint8_t mode = iodev->mode;

    // Not ready if busy writing.
    if (port_data->tx_busy) {
        return false;
    }

    uint32_t time = pbdrv_clock_get_ms();

    // If we were setting data, then wait for the matching delay.
    if (port_data->tx_type == LUMP_MSG_TYPE_DATA) {
        return time - port_data->tx_start_time >= pbio_iodev_delay_set_data(id, mode);
    }

    // Ready if mode change is complete and we have waited long enough for stale data to be discarded.
    return mode == port_data->new_mode &&
           time - port_data->mode_switch_time >= pbio_iodev_delay_stale_data(id, mode);
}

/**
 * Starts sending data to the LEGO UART device mode.
 *
 * @param [in]  iodev       The I/O device
 * @return                  Error code corresponding to ::ev3_uart_begin_tx_msg
 *                          or ::PBIO_ERROR_INVALID_OP if device not in expected mode.
 */
static pbio_error_t pbio_uartdev_start_buffered_data_set(pbio_iodev_t *iodev) {

    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    const pbio_iodev_mode_t *mode_info = &iodev->info->mode_info[iodev->mode];

    // Reset data length so we transmit only once.
    uint8_t size = port_data->data_set_len;
    port_data->data_set_len = 0;

    // Not all modes support setting data and data must be of expected size.
    if (!(mode_info->data_type & PBIO_IODEV_DATA_TYPE_WRITABLE) ||
        size != mode_info->num_values * pbio_iodev_size_of(mode_info->data_type)) {
        return PBIO_ERROR_INVALID_OP;
    }

    return ev3_uart_begin_tx_msg(port_data, LUMP_MSG_TYPE_DATA, iodev->mode, iodev->bin_data, size);
}

/**
 * Starts sending data to the LEGO UART device mode if there is any.
 */
static void pbio_uartdev_handle_data_set_start(pbio_iodev_t *iodev) {
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    if (pbio_uartdev_operation_complete(iodev) && port_data->data_set_len > 0) {
        pbio_uartdev_start_buffered_data_set(iodev);
    }
}

static void pbio_uartdev_handle_write_end(pbio_iodev_t *iodev) {

    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    if (!port_data->tx_busy) {
        return;
    }

    pbio_error_t err = pbdrv_uart_write_end(port_data->uart);
    if (err != PBIO_ERROR_AGAIN) {
        port_data->tx_busy = false;
    }
}

static PT_THREAD(pbio_uartdev_init(struct pt *pt, uint8_t id)) {
    const pbio_uartdev_platform_data_t *pdata = &pbio_uartdev_platform_data[id];
    uartdev_port_data_t *port_data = &dev_data[id];

    PT_BEGIN(pt);

    PT_WAIT_UNTIL(pt, pbdrv_uart_get(pdata->uart_id, &port_data->uart) == PBIO_SUCCESS);
    port_data->iodev.info = &infos[id].info;
    // FIXME: uartdev should not have to care about port numbers
    port_data->iodev.port = PBIO_CONFIG_UARTDEV_FIRST_PORT + id;
    port_data->info = &infos[id].info;
    port_data->tx_msg = &bufs[id][BUF_TX_MSG][0];
    port_data->rx_msg = &bufs[id][BUF_RX_MSG][0];

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
            pbio_uartdev_handle_write_end(&data->iodev);
            if (data->status == PBIO_UARTDEV_STATUS_DATA) {
                pbio_uartdev_handle_data_set_start(&data->iodev);
            }
        }
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

/**
 * Checks if LEGO UART device has data available for reading or is ready to write.
 *
 * @param [in]  iodev       The I/O device
 * @return                  ::PBIO_SUCCESS if ready.
 *                          ::PBIO_ERROR_AGAIN if not ready yet.
 *                          ::PBIO_ERROR_NO_DEV if no device is attached.
 */
pbio_error_t pbio_iodev_is_ready(pbio_iodev_t *iodev) {

    // Device is not there or still syncing.
    if (iodev->info->type_id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    // Ready if operations are complete and there is no data left to set.
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    return pbio_uartdev_operation_complete(iodev) && port_data->data_set_len == 0 ? PBIO_SUCCESS : PBIO_ERROR_AGAIN;
}

/**
 * Starts setting the mode of a LEGO UART device.
 *
 * @param [in]  iodev       The I/O device.
 * @param [in]  id          The ID of the device to request data from.
 * @param [in]  mode        The mode to set.
 * @return                  ::PBIO_SUCCESS on success or if mode already set.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 *                          ::PBIO_ERROR_INVALID_ARG if the mode is not valid.
 *                          ::PBIO_ERROR_AGAIN if the device is not ready for this operation.
 */
pbio_error_t pbio_iodev_set_mode(pbio_iodev_t *iodev, uint8_t mode) {

    // Device is not there or still syncing.
    if (iodev->info->type_id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    // Can only set available modes.
    if (mode >= iodev->info->num_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }

    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);

    // Discard any old data that was never sent.
    port_data->data_set_len = 0;

    // Mode already set or being set, so return success.
    if (port_data->new_mode == mode || iodev->mode == mode) {
        return PBIO_SUCCESS;
    }

    // We can only initiate a mode switch if currently idle (receiving data).
    pbio_error_t err = pbio_iodev_is_ready(iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start setting new mode.
    err = ev3_uart_begin_tx_msg(port_data, LUMP_MSG_TYPE_CMD, LUMP_CMD_SELECT, &mode, 1);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    port_data->new_mode = mode;
    return PBIO_SUCCESS;
}

/**
 * Atomic operation for asserting the mode/id and getting the data of a LEGO UART device.
 *
 * The returned data buffer is 4-byte aligned. Data is in little-endian format.
 *
 * @param [in]  iodev       The I/O device
 * @param [out] data        Pointer to hold array of data values.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 *                          ::PBIO_ERROR_AGAIN if the device is not ready for this operation.
 */
pbio_error_t pbio_iodev_get_data(pbio_iodev_t *iodev, uint8_t mode, void **data) {

    // Device is not there or still syncing.
    if (iodev->info->type_id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);

    // Can only request data for mode that is set.
    if (mode != port_data->iodev.mode) {
        return PBIO_ERROR_INVALID_OP;
    }

    pbio_error_t err = pbio_iodev_is_ready(iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *data = iodev->bin_data;

    return PBIO_SUCCESS;
}

/**
 * Set data for the current mode.
 *
 * @param [in]  iodev       The I/O device
 * @param [out] data        Data to be set.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 */
pbio_error_t pbio_iodev_set_mode_with_data(pbio_iodev_t *iodev, uint8_t mode, const void *data) {

    // Start setting mode.
    pbio_error_t err = pbio_iodev_set_mode(iodev, mode);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // info of the mode *to be set*, which might not be current mode.
    uartdev_port_data_t *port_data = PBIO_CONTAINER_OF(iodev, uartdev_port_data_t, iodev);
    pbio_iodev_mode_t *mode_info = &port_data->info->mode_info[mode];

    // Check if the device is in this mode already.
    err = pbio_iodev_is_ready(iodev);
    if (err != PBIO_SUCCESS && err != PBIO_ERROR_AGAIN) {
        return err;
    }

    // Copy data to be sent after mode switch.
    port_data->data_set_len = mode_info->num_values * pbio_iodev_size_of(mode_info->data_type);
    memcpy(iodev->bin_data, data, port_data->data_set_len);

    // If already in the right mode, start sending data right away.
    if (err == PBIO_SUCCESS) {
        return pbio_uartdev_start_buffered_data_set(iodev);
    }
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_UARTDEV

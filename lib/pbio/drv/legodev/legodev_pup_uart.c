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

#include <pbdrv/config.h>

#include <pbdrv/legodev.h>

#if PBDRV_CONFIG_LEGODEV_PUP_UART

#include "legodev_pup_uart.h"
#include "legodev_spec.h"

#define DEBUG 0
#if DEBUG
#include <inttypes.h>
#define debug_pr(fmt, ...)   printf((fmt), __VA_ARGS__)
#define debug_pr_str(str)   printf((str))
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define debug_pr_str(...)
#define DBG_ERR(expr)
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>
#include <lego_uart.h>

#include "../drv/legodev/legodev_pup.h"

#include "pbdrv/config.h"
#include "pbdrv/uart.h"
#include "pbio/error.h"
#include "pbio/event.h"
#include "pbio/port.h"
#include "pbio/util.h"

#include <pbio/dcmotor.h>

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
    PBDRV_LEGODEV_PUP_UART_STATUS_ERR,
    /**< Waiting for data that looks like LEGO UART protocol. */
    PBDRV_LEGODEV_PUP_UART_STATUS_SYNCING,
    /**< Reading device info before changing baud rate. */
    PBDRV_LEGODEV_PUP_UART_STATUS_INFO,
    /**< ACK received, delay changing baud rate. */
    PBDRV_LEGODEV_PUP_UART_STATUS_ACK,
    /**< Ready to send commands and receive data. */
    PBDRV_LEGODEV_PUP_UART_STATUS_DATA,
} pbdrv_legodev_pup_uart_status_t;

typedef struct {
    /**< The mode to be set. */
    uint8_t desired_mode;
    /**< Whether a mode change was requested (set low when handled). */
    bool requested;
    /**< Time of switch completion (if info.mode == desired_mode) or time of switch request (if info.mode != desired_mode). */
    uint32_t time;
} pbdrv_legodev_pup_uart_mode_switch_t;

typedef struct {
    /**< The data to be set. */
    uint8_t bin_data[PBDRV_LEGODEV_MAX_DATA_SIZE]  __attribute__((aligned(4)));
    /**< The size of the data to be set, also acts as set request flag. */
    uint8_t size;
    /**< The mode at which to set data */
    uint8_t desired_mode;
    /**< Time of the data set request (if size != 0) or time of completing transmission (if size == 0). */
    uint32_t time;
} pbdrv_legodev_pup_uart_data_set_t;

/**
 * struct ev3_uart_port_data - Data for EV3/LPF2 UART Sensor communication
 */
struct _pbdrv_legodev_pup_uart_dev_t {
    /**< Protothread for main communication protocol. */
    struct pt pt;
    /**< Child protothread of the main protothread used for writing data */
    struct pt write_pt;
    /**< Protothread for receiving sensor data. */
    struct pt data_pt;
    /**< Timer for sending keepalive messages and other delays. */
    struct etimer timer;
    /**< Device information, including mode info */
    pbdrv_legodev_info_t device_info;
    /**< Pointer to the UART device to use for communications. */
    pbdrv_uart_dev_t *uart;
    /**< Pointer to the DC motor device to use for powered devices. */
    pbio_dcmotor_t *dcmotor;
    /**
     * Most recent binary data read from the device. How to interpret this data
     * is determined by the ::pbdrv_legodev_mode_info_t info associated with the current
     * *mode* of the device. For example, it could be an array of int32_t and/or
     * the values could be foreign-endian.
     */
    uint8_t *bin_data;
    /**< The current device connection state. */
    pbdrv_legodev_pup_uart_status_t status;
    /**< Mode switch status. */
    pbdrv_legodev_pup_uart_mode_switch_t mode_switch;
    /**< Data set buffer and status. */
    pbdrv_legodev_pup_uart_data_set_t *data_set;
    /**< Extra mode adder for Powered Up devices (for modes > LUMP_MAX_MODE). */
    uint8_t ext_mode;
    /**< New baud rate that will be set with ev3_uart_change_bitrate. */
    uint32_t new_baud_rate;
    /**< Buffer to hold messages transmitted to the device. */
    uint8_t *tx_msg;
    /**< Size of the current message being transmitted. */
    uint8_t tx_msg_size;
    /**< Buffer to hold messages received from the device. */
    uint8_t *rx_msg;
    /**< Size of the current message being received. */
    uint8_t rx_msg_size;
    /**< Total number of errors that have occurred. */
    uint32_t err_count;
    /**< Number of bad reads when receiving DATA data->msgs. */
    uint32_t num_data_err;
    /**< Time of most recently started transmission. */
    uint32_t tx_start_time;
    /**< Flag that indicates that good DATA data->msg has been received since last watchdog timeout. */
    bool data_rec;
    /**< data->msg to be printed in case of an error. */
    DBG_ERR(const char *last_err);
    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    /**< Mode value used to keep track of mode in INFO messages while syncing. */
    uint8_t new_mode;
    /**< Flags indicating what information has already been read from the data. */
    uint32_t info_flags;
    #endif // #define PBDRV_CONFIG_LEGODEV_MODE_INFO
};

enum {
    BUF_TX_MSG,
    BUF_RX_MSG,
    NUM_BUF
};

static uint8_t bufs[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV][NUM_BUF][EV3_UART_MAX_MESSAGE_SIZE];

static pbdrv_legodev_pup_uart_dev_t dev_data[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV];

// The following data is really just part of dev_data, but separate allocation reduces overal code size
static uint8_t data_read_bufs[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV][PBDRV_LEGODEV_MAX_DATA_SIZE] __attribute__((aligned(4)));
static pbdrv_legodev_pup_uart_data_set_t data_set_bufs[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV];

#define PBIO_PT_WAIT_READY(pt, expr) PT_WAIT_UNTIL((pt), (expr) != PBIO_ERROR_AGAIN)

pbdrv_legodev_pup_uart_dev_t *pbdrv_legodev_pup_uart_configure(uint8_t device_index, uint8_t uart_driver_index, pbio_dcmotor_t *dcmotor) {
    pbdrv_legodev_pup_uart_dev_t *port_data = &dev_data[device_index];
    port_data->dcmotor = dcmotor;
    port_data->tx_msg = &bufs[device_index][BUF_TX_MSG][0];
    port_data->rx_msg = &bufs[device_index][BUF_RX_MSG][0];
    port_data->status = PBDRV_LEGODEV_PUP_UART_STATUS_ERR;
    port_data->err_count = 0;
    port_data->data_set = &data_set_bufs[device_index];
    port_data->bin_data = data_read_bufs[device_index];

    // legodev driver is started after all other drivers, so we
    // assume that we do not need to wait for this to be ready.
    pbdrv_uart_get(uart_driver_index, &port_data->uart);
    return port_data;
}

static void pbdrv_legodev_request_mode(pbdrv_legodev_pup_uart_dev_t *port_data, uint8_t mode) {
    port_data->mode_switch.desired_mode = mode;
    port_data->mode_switch.time = pbdrv_clock_get_ms();
    port_data->mode_switch.requested = true;
    pbdrv_legodev_pup_uart_process_poll();
}

static void pbdrv_legodev_request_data_set(pbdrv_legodev_pup_uart_dev_t *port_data, uint8_t mode, const uint8_t *data, uint8_t size) {
    port_data->data_set->size = size;
    port_data->data_set->desired_mode = mode;
    port_data->data_set->time = pbdrv_clock_get_ms();
    memcpy(port_data->data_set->bin_data, data, size);
    pbdrv_legodev_pup_uart_process_poll();
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


static void pbdrv_legodev_pup_uart_parse_msg(pbdrv_legodev_pup_uart_dev_t *data) {
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
            if (data->status == PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {

                // The LEGO EV3 color sensor sends bad checksums
                // for RGB-RAW data (mode 4). The check here could be
                // improved if someone can find a pattern.
                if (data->device_info.type_id != PBDRV_LEGODEV_TYPE_ID_EV3_COLOR_SENSOR
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
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (!data->device_info.num_modes) {
                        DBG_ERR(data->last_err = "Received ACK before all mode INFO");
                        goto err;
                    }
                    if ((data->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                        DBG_ERR(data->last_err = "Did not receive all required INFO");
                        goto err;
                    }
                    data->device_info.mode = data->new_mode;
                    #endif

                    data->status = PBDRV_LEGODEV_PUP_UART_STATUS_ACK;

                    return;
            }
            break;
        case LUMP_MSG_TYPE_CMD:
            switch (cmd) {
                case LUMP_CMD_MODES:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_MODES, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate modes INFO");
                        goto err;
                    }
                    if (cmd2 > LUMP_MAX_MODE) {
                        DBG_ERR(data->last_err = "Number of modes is out of range");
                        goto err;
                    }
                    data->device_info.num_modes = cmd2 + 1;
                    if (msg_size > 5) {
                        // Powered Up devices can have an extended mode message that
                        // includes modes > LUMP_MAX_MODE
                        data->device_info.num_modes = data->rx_msg[3] + 1;
                    }

                    debug_pr("num_modes: %d\n", data->device_info.num_modes);
                    #endif
                    break;
                case LUMP_CMD_SPEED:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_SPEED, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate speed INFO");
                        goto err;
                    }
                    #endif
                    speed = pbio_get_uint32_le(data->rx_msg + 1);
                    if (speed < EV3_UART_SPEED_MIN || speed > EV3_UART_SPEED_MAX) {
                        DBG_ERR(data->last_err = "Speed is out of range");
                        goto err;
                    }
                    data->new_baud_rate = speed;

                    debug_pr("speed: %" PRIu32 "\n", speed);

                    break;
                case LUMP_CMD_WRITE:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (cmd2 & 0x20) {
                        // TODO: write_cmd_size = cmd2 & 0x3;
                        if (data->info_flags & PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS) {
                            // TODO: msg[3] and msg[4] probably give us useful information
                        } else {
                            // TODO: handle other write commands
                        }
                    }
                    #endif
                    break;
                case LUMP_CMD_EXT_MODE:
                    // Powered up devices can have modes > LUMP_MAX_MODE. This
                    // command precedes other commands to add the extra 8 to the mode
                    data->ext_mode = data->rx_msg[1];
                    break;
                case LUMP_CMD_VERSION:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
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
        #if PBDRV_CONFIG_LEGODEV_MODE_INFO
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
                    const char *name = (char *)(data->rx_msg + 2);
                    size_t name_len = strlen(name);
                    if (name_len > LUMP_MAX_NAME_SIZE) {
                        DBG_ERR(data->last_err = "Name is too long");
                        goto err;
                    }
                    data->new_mode = mode;
                    data->info_flags |= EV3_UART_INFO_FLAG_INFO_NAME;
                    strncpy(data->device_info.mode_info[mode].name, name, name_len);

                    // newer LEGO UART devices send additional 6 mode capability flags
                    uint8_t flags = 0;
                    if (name_len <= LUMP_MAX_SHORT_NAME_SIZE && msg_size > LUMP_MAX_NAME_SIZE) {
                        // Only the first is used in practice.
                        flags = data->rx_msg[8];
                    } else {
                        // for newer devices that don't send it, set flags by device ID
                        // TODO: Look up from static info like we do for basic devices
                        if (data->device_info.type_id == PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR) {
                            flags = LUMP_MODE_FLAGS0_MOTOR | LUMP_MODE_FLAGS0_MOTOR_POWER | LUMP_MODE_FLAGS0_MOTOR_SPEED | LUMP_MODE_FLAGS0_MOTOR_REL_POS;
                        }
                    }

                    // Although capabilities are sent per mode, we apply them to the whole device
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_POWER) {
                        data->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_IS_DC_OUTPUT;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_SPEED) {
                        data->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_SPEED;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_REL_POS) {
                        data->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_ABS_POS) {
                        data->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS;
                    }
                    if (flags & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN1) {
                        data->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1;
                    }
                    if (flags & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN2) {
                        data->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2;
                    }

                    debug_pr("new_mode: %d\n", data->new_mode);
                    debug_pr("flags: %02X %02X %02X %02X %02X %02X\n",
                        data->rx_msg[8 + 0], data->rx_msg[8 + 1], data->rx_msg[8 + 2],
                        data->rx_msg[8 + 3], data->rx_msg[8 + 4], data->rx_msg[8 + 5]);
                }
                break;
                case LUMP_INFO_RAW:
                case LUMP_INFO_PCT:
                case LUMP_INFO_SI:
                case LUMP_INFO_UNITS:
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

                    // Mode supports writing if rx_msg[3] is nonzero.
                    data->device_info.mode_info[mode].writable = data->rx_msg[3] != 0;

                    debug_pr("mapping: in %02x out %02x\n", data->rx_msg[2], data->rx_msg[3]);
                    debug_pr("mapping: in %02x out %02x\n", data->rx_msg[2], data->rx_msg[3]);
                    debug_pr("Writable: %d\n", data->device_info.mode_info[mode].writable);

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
                case LUMP_INFO_FORMAT:
                    if (data->new_mode != mode) {
                        DBG_ERR(data->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_FORMAT, &data->info_flags)) {
                        DBG_ERR(data->last_err = "Received duplicate format INFO");
                        goto err;
                    }
                    data->device_info.mode_info[mode].num_values = data->rx_msg[2];
                    if (!data->device_info.mode_info[mode].num_values) {
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
                    data->device_info.mode_info[mode].data_type = data->rx_msg[3];
                    if (data->new_mode) {
                        data->new_mode--;
                    }

                    debug_pr("num_values: %d\n", data->device_info.mode_info[mode].num_values);
                    debug_pr("data_type: %d\n", data->device_info.mode_info[mode].data_type);

                    break;
            }
            break;
        #endif
        case LUMP_MSG_TYPE_DATA:
            if (data->status != PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {
                DBG_ERR(data->last_err = "Received DATA before INFO was complete");
                goto err;
            }

            #if PBDRV_CONFIG_LEGODEV_MODE_INFO
            if (mode >= data->device_info.num_modes) {
                DBG_ERR(data->last_err = "Invalid mode received");
                goto err;
            }
            #endif

            // Data is for requested mode.
            if (mode == data->mode_switch.desired_mode) {
                memcpy(data->bin_data, data->rx_msg + 1, msg_size - 2);

                if (data->device_info.mode != mode) {
                    // First time getting data in this mode, so register time.
                    data->mode_switch.time = pbdrv_clock_get_ms();
                }
            }
            data->device_info.mode = mode;

            data->data_rec = true;
            if (data->num_data_err) {
                data->num_data_err--;
            }
            break;
    }

    return;

err:
    data->status = PBDRV_LEGODEV_PUP_UART_STATUS_ERR;
    debug_pr("Data error: %s\n", data->last_err);
}

static uint8_t ev3_uart_set_msg_hdr(lump_msg_type_t type, lump_msg_size_t size, lump_cmd_t cmd) {
    return (type & LUMP_MSG_TYPE_MASK) | (size & LUMP_MSG_SIZE_MASK) | (cmd & LUMP_MSG_CMD_MASK);
}

static void ev3_uart_prepare_tx_msg(pbdrv_legodev_pup_uart_dev_t *port_data, lump_msg_type_t msg_type,
    lump_cmd_t cmd, const uint8_t *data, uint8_t len) {
    uint8_t header, checksum, i;
    uint8_t offset = 0;
    lump_msg_size_t size;

    if (msg_type == LUMP_MSG_TYPE_DATA) {
        // Only Powered Up devices support setting data, and they expect to have an
        // extra command sent to give the part of the mode > 7
        port_data->tx_msg[0] = ev3_uart_set_msg_hdr(LUMP_MSG_TYPE_CMD, LUMP_MSG_SIZE_1, LUMP_CMD_EXT_MODE);
        port_data->tx_msg[1] = port_data->device_info.mode > LUMP_MAX_MODE ? 8 : 0;
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
    port_data->tx_msg_size = offset + i + 2;
}

static PT_THREAD(pbdrv_legodev_pup_uart_send_prepared_msg(pbdrv_legodev_pup_uart_dev_t * data, pbio_error_t * err)) {
    PT_BEGIN(&data->write_pt);
    data->tx_start_time = pbdrv_clock_get_ms();
    PBIO_PT_WAIT_READY(&data->write_pt, *err = pbdrv_uart_write_begin(data->uart, data->tx_msg, data->tx_msg_size, EV3_UART_IO_TIMEOUT));
    if (*err == PBIO_SUCCESS) {
        PBIO_PT_WAIT_READY(&data->write_pt, *err = pbdrv_uart_write_end(data->uart));
    }
    PT_END(&data->write_pt);
}

static PT_THREAD(pbdrv_legodev_pup_uart_update(pbdrv_legodev_pup_uart_dev_t * data)) {
    static pbio_error_t err;
    uint8_t checksum;

    PT_BEGIN(&data->pt);

    // reset state for new device
    data->device_info.type_id = PBDRV_LEGODEV_TYPE_ID_NONE;
    data->device_info.mode = 0;
    data->ext_mode = 0;
    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    data->device_info.flags = PBDRV_LEGODEV_CAPABILITY_FLAG_NONE;
    #endif
    data->status = PBDRV_LEGODEV_PUP_UART_STATUS_SYNCING;

    // Send SPEED command at 115200 baud
    debug_pr("set baud: %d\n", EV3_UART_SPEED_LPF2);
    pbdrv_uart_set_baud_rate(data->uart, EV3_UART_SPEED_LPF2);
    uint8_t speed_payload[4];
    pbio_set_uint32_le(speed_payload, EV3_UART_SPEED_LPF2);
    ev3_uart_prepare_tx_msg(data, LUMP_MSG_TYPE_CMD, LUMP_CMD_SPEED, speed_payload, sizeof(speed_payload));

    pbdrv_uart_flush(data->uart);

    PT_SPAWN(&data->pt, &data->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(data, &err));
    if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "Speed tx failed");
        goto err;
    }

    pbdrv_uart_flush(data->uart);

    // read one byte to check for ACK
    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_begin(data->uart, data->rx_msg, 1, 10));
    if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "UART Rx error during baud");
        goto err;
    }

    PBIO_PT_WAIT_READY(&data->pt, err = pbdrv_uart_read_end(data->uart));
    if ((err == PBIO_SUCCESS && data->rx_msg[0] != LUMP_SYS_ACK) || err == PBIO_ERROR_TIMEDOUT) {
        // if we did not get ACK within 100ms, then switch to slow baud rate for sync
        pbdrv_uart_set_baud_rate(data->uart, EV3_UART_SPEED_MIN);
        debug_pr("set baud: %d\n", EV3_UART_SPEED_MIN);
    } else if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "UART Rx error during baud");
        goto err;
    }

sync:

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

    bool bad_id = data->rx_msg[1] < EV3_UART_TYPE_MIN || data->rx_msg[1] > EV3_UART_TYPE_MAX;
    checksum = 0xff ^ data->rx_msg[0] ^ data->rx_msg[1];
    bool bad_id_checksum = data->rx_msg[2] != checksum;

    if (bad_id || bad_id_checksum) {
        DBG_ERR(data->last_err = "Bad device type id or checksum");
        if (data->err_count > 10) {
            data->err_count = 0;
            goto err;
        }
        data->err_count++;
        goto sync;
    }

    // if all was good, we are ready to start receiving the mode info
    data->device_info.type_id = data->rx_msg[1];
    data->data_rec = false;
    data->num_data_err = 0;
    data->status = PBDRV_LEGODEV_PUP_UART_STATUS_INFO;
    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    data->device_info.flags = PBDRV_LEGODEV_CAPABILITY_FLAG_NONE;
    data->info_flags = EV3_UART_INFO_FLAG_CMD_TYPE;
    data->device_info.num_modes = 1;
    #endif
    debug_pr("type id: %d\n", data->device_info.type_id);

    while (data->status == PBDRV_LEGODEV_PUP_UART_STATUS_INFO) {
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
        pbdrv_legodev_pup_uart_parse_msg(data);
    }

    // at this point we should have read all of the mode info
    if (data->status != PBDRV_LEGODEV_PUP_UART_STATUS_ACK) {
        // data->last_err should be set by pbdrv_legodev_pup_uart_parse_msg()
        goto err;
    }

    // reply with ACK
    data->tx_msg[0] = LUMP_SYS_ACK;
    data->tx_msg_size = 1;
    PT_SPAWN(&data->pt, &data->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(data, &err));
    if (err != PBIO_SUCCESS) {
        DBG_ERR(data->last_err = "UART Tx error during ack.");
        goto err;
    }

    // schedule baud rate change
    etimer_set(&data->timer, 10);
    PT_WAIT_UNTIL(&data->pt, etimer_expired(&data->timer));

    // change the baud rate
    pbdrv_uart_set_baud_rate(data->uart, data->new_baud_rate);
    debug_pr("set baud: %" PRIu32 "\n", data->new_baud_rate);

    data->status = PBDRV_LEGODEV_PUP_UART_STATUS_DATA;
    // reset data rx thread
    PT_INIT(&data->data_pt);

    // Load static flags on platforms that don't read device info
    #if !PBDRV_CONFIG_LEGODEV_MODE_INFO
    data->device_info.flags = pbdrv_legodev_spec_basic_flags(data->device_info.type_id);
    #endif

    // Turn on power for devices that need it
    if (data->device_info.flags & PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1) {
        pbdrv_motor_driver_set_duty_cycle(data->dcmotor->motor_driver, -PBDRV_MOTOR_DRIVER_MAX_DUTY);
    } else if (data->device_info.flags & PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2) {
        pbdrv_motor_driver_set_duty_cycle(data->dcmotor->motor_driver, PBDRV_MOTOR_DRIVER_MAX_DUTY);
    } else {
        pbdrv_motor_driver_coast(data->dcmotor->motor_driver);
    }

    // Request switch to default mode for this device.
    pbdrv_legodev_request_mode(data, pbdrv_legodev_spec_default_mode(data->device_info.type_id));

    // Reset other timers
    etimer_reset_with_new_interval(&data->timer, EV3_UART_DATA_KEEP_ALIVE_TIMEOUT);
    data->data_set->time = pbdrv_clock_get_ms();
    data->data_set->size = 0;

    while (data->status == PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {

        PT_WAIT_UNTIL(&data->pt, etimer_expired(&data->timer) || data->mode_switch.requested || data->data_set->size > 0);

        // Handle keep alive timeout
        if (etimer_expired(&data->timer)) {
            // make sure we are receiving data
            if (!data->data_rec) {
                data->num_data_err++;
                DBG_ERR(data->last_err = "No data since last keepalive");
                if (data->num_data_err > 6) {
                    data->status = PBDRV_LEGODEV_PUP_UART_STATUS_ERR;
                    goto err;
                }
            }
            data->data_rec = false;
            data->tx_msg[0] = LUMP_SYS_NACK;
            data->tx_msg_size = 1;
            PT_SPAWN(&data->pt, &data->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(data, &err));
            if (err != PBIO_SUCCESS) {
                DBG_ERR(data->last_err = "Error during keepalive.");
                goto err;
            }
            etimer_reset_with_new_interval(&data->timer, EV3_UART_DATA_KEEP_ALIVE_TIMEOUT);

            // Retry mode switch if it hasn't been handled or failed.
            if (data->device_info.mode != data->mode_switch.desired_mode && pbdrv_clock_get_ms() - data->mode_switch.time > EV3_UART_IO_TIMEOUT) {
                data->mode_switch.requested = true;
            }
        }

        // Handle requested mode change
        if (data->mode_switch.requested) {
            data->mode_switch.requested = false;
            ev3_uart_prepare_tx_msg(data, LUMP_MSG_TYPE_CMD, LUMP_CMD_SELECT, &data->mode_switch.desired_mode, 1);
            PT_SPAWN(&data->pt, &data->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(data, &err));
            if (err != PBIO_SUCCESS) {
                DBG_ERR(data->last_err = "Setting requested mode failed.");
                goto err;
            }
        }

        // Handle requested data set
        if (data->data_set->size > 0) {
            // Only set data if we are in the correct mode already.
            if (data->device_info.mode == data->data_set->desired_mode) {
                ev3_uart_prepare_tx_msg(data, LUMP_MSG_TYPE_DATA, data->data_set->desired_mode, data->data_set->bin_data, data->data_set->size);
                data->data_set->size = 0;
                data->data_set->time = pbdrv_clock_get_ms();
                PT_SPAWN(&data->pt, &data->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(data, &err));
                if (err != PBIO_SUCCESS) {
                    DBG_ERR(data->last_err = "Setting requested data failed.");
                    goto err;
                }
                data->data_set->time = pbdrv_clock_get_ms();
            } else if (pbdrv_clock_get_ms() - data->data_set->time < 500) {
                // Not in the right mode yet, try again later for a reasonable amount of time.
                pbdrv_legodev_pup_uart_process_poll();
                PT_YIELD(&data->pt);
            } else {
                // Give up setting data.
                data->data_set->size = 0;
            }
        }
    }

err:
    // reset and start over
    data->status = PBDRV_LEGODEV_PUP_UART_STATUS_ERR;
    etimer_stop(&data->timer);
    debug_pr("%s\n", data->last_err);

    // Turn off battery supply to this port
    pbdrv_motor_driver_coast(data->dcmotor->motor_driver);

    PT_END(&data->pt);
}

// REVISIT: This is not the greatest. We can easily get a buffer overrun and
// loose data. For now, the retry after bad message size helps get back into
// sync with the data stream.
static PT_THREAD(pbdrv_legodev_pup_uart_receive_data(pbdrv_legodev_pup_uart_dev_t * data)) {
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
        pbdrv_legodev_pup_uart_parse_msg(data);
    }

    PT_END(&data->data_pt);
}

/**
 * Gets the size of a data type.
 * @param [in]  type        The data type
 * @return                  The size of the type or 0 if the type was not valid
 */
size_t pbdrv_legodev_size_of(pbdrv_legodev_data_type_t type) {
    switch (type) {
        case PBDRV_LEGODEV_DATA_TYPE_INT8:
            return 1;
        case PBDRV_LEGODEV_DATA_TYPE_INT16:
            return 2;
        case PBDRV_LEGODEV_DATA_TYPE_INT32:
        case PBDRV_LEGODEV_DATA_TYPE_FLOAT:
            return 4;
    }
    return 0;
}

PT_THREAD(pbdrv_legodev_pup_uart_thread(struct pt *pt, pbdrv_legodev_pup_uart_dev_t *port_data)) {
    PT_BEGIN(pt);
    PT_INIT(&port_data->pt);
    while (PT_SCHEDULE(pbdrv_legodev_pup_uart_update(port_data))) {
        if (port_data->status == PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {
            pbdrv_legodev_pup_uart_receive_data(port_data);
        }
        PT_YIELD(pt);
    }

    PT_END(pt);
}

/**
 * Checks if LEGO UART device has data available for reading or is ready to write.
 *
* @param [in]  legodev     The legodev instance.
 * @return                  ::PBIO_SUCCESS if ready.
 *                          ::PBIO_ERROR_AGAIN if not ready yet.
 *                          ::PBIO_ERROR_NO_DEV if no device is attached.
 */
pbio_error_t pbdrv_legodev_is_ready(pbdrv_legodev_dev_t *legodev) {

    pbdrv_legodev_pup_uart_dev_t *port_data = pbdrv_legodev_get_uart_dev(legodev);
    if (!port_data || port_data->status == PBDRV_LEGODEV_PUP_UART_STATUS_ERR) {
        return PBIO_ERROR_NO_DEV;
    }

    if (port_data->status != PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {
        return PBIO_ERROR_AGAIN;
    }

    uint32_t time = pbdrv_clock_get_ms();

    // Not ready if waiting for mode change
    if (port_data->device_info.mode != port_data->mode_switch.desired_mode) {
        return PBIO_ERROR_AGAIN;
    }

    // Not ready if waiting for stale data to be discarded.
    if (time - port_data->mode_switch.time <= pbdrv_legodev_spec_stale_data_delay(port_data->device_info.type_id, port_data->device_info.mode)) {
        return PBIO_ERROR_AGAIN;
    }

    // Not ready if just recently set new data.
    if (port_data->data_set->size > 0 || time - port_data->data_set->time <= pbdrv_legodev_spec_data_set_delay(port_data->device_info.type_id, port_data->device_info.mode)) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Starts setting the mode of a LEGO UART device.
 *
* @param [in]  legodev     The legodev instance..
 * @param [in]  id          The ID of the device to request data from.
 * @param [in]  mode        The mode to set.
 * @return                  ::PBIO_SUCCESS on success or if mode already set.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 *                          ::PBIO_ERROR_INVALID_ARG if the mode is not valid.
 *                          ::PBIO_ERROR_AGAIN if the device is not ready for this operation.
 */
pbio_error_t pbdrv_legodev_set_mode(pbdrv_legodev_dev_t *legodev, uint8_t mode) {

    pbdrv_legodev_pup_uart_dev_t *port_data = pbdrv_legodev_get_uart_dev(legodev);
    if (!port_data) {
        return PBIO_ERROR_NO_DEV;
    }

    // Mode already set or being set, so return success.
    if (port_data->mode_switch.desired_mode == mode || port_data->device_info.mode == mode) {
        return PBIO_SUCCESS;
    }

    // We can only initiate a mode switch if currently idle (receiving data).
    pbio_error_t err = pbdrv_legodev_is_ready(legodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    // Can only set available modes.
    if (mode >= port_data->device_info.num_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }
    #endif

    // Request mode switch.
    pbdrv_legodev_request_mode(port_data, mode);

    return PBIO_SUCCESS;
}

/**
 * Atomic operation for asserting the mode/id and getting the data of a LEGO UART device.
 *
 * The returned data buffer is 4-byte aligned. Data is in little-endian format.
 *
 * @param [in]  legodev     The legodev instance.
 * @param [out] data        Pointer to hold array of data values.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 *                          ::PBIO_ERROR_AGAIN if the device is not ready for this operation.
 */
pbio_error_t pbdrv_legodev_get_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, void **data) {

    pbdrv_legodev_pup_uart_dev_t *port_data = pbdrv_legodev_get_uart_dev(legodev);
    if (!port_data) {
        return PBIO_ERROR_NO_DEV;
    }

    // Can only request data for mode that is set.
    if (mode != port_data->device_info.mode) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Can only read if ready.
    *data = port_data->bin_data;
    return pbdrv_legodev_is_ready(legodev);
}

/**
 * Set data for the current mode.
 *
* @param [in]  legodev     The legodev instance.
 * @param [out] data        Data to be set.
 * @param [in]  size        Size of data to be set.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 */
pbio_error_t pbdrv_legodev_set_mode_with_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, const void *data, uint8_t size) {

    pbdrv_legodev_pup_uart_dev_t *port_data = pbdrv_legodev_get_uart_dev(legodev);
    if (!port_data) {
        return PBIO_ERROR_NO_DEV;
    }

    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    const pbdrv_legodev_mode_info_t *mode_info = &port_data->device_info.mode_info[mode];
    // Not all modes support setting data and data must be of expected size.
    if (!mode_info->writable || size != mode_info->num_values * pbdrv_legodev_size_of(mode_info->data_type)) {
        return PBIO_ERROR_INVALID_OP;
    }
    #endif

    // Start setting mode.
    pbio_error_t err = pbdrv_legodev_set_mode(legodev, mode);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Request data set.
    pbdrv_legodev_request_data_set(port_data, mode, data, size);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_info(pbdrv_legodev_dev_t *legodev, pbdrv_legodev_info_t **info) {

    pbdrv_legodev_pup_uart_dev_t *port_data = pbdrv_legodev_get_uart_dev(legodev);
    if (!port_data) {
        return PBIO_ERROR_NO_DEV;
    }
    // Info is set even in case of error. Caller can decide what values apply
    // based on the error code.
    *info = &port_data->device_info;
    return pbdrv_legodev_is_ready(legodev);
}

#endif // PBDRV_CONFIG_LEGODEV_PUP_UART

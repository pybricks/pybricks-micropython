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
    /** Waiting for data that looks like LEGO UART protocol. */
    PBDRV_LEGODEV_PUP_UART_STATUS_SYNCING,
    /** Reading device info before changing baud rate. */
    PBDRV_LEGODEV_PUP_UART_STATUS_INFO,
    /** ACK received, delay changing baud rate. */
    PBDRV_LEGODEV_PUP_UART_STATUS_ACK,
    /** Ready to send commands and receive data. */
    PBDRV_LEGODEV_PUP_UART_STATUS_DATA,
} pbdrv_legodev_pup_uart_status_t;

typedef struct {
    /** The mode to be set. */
    uint8_t desired_mode;
    /** Whether a mode change was requested (set low when handled). */
    bool requested;
    /** Time of switch completion (if info.mode == desired_mode) or time of switch request (if info.mode != desired_mode). */
    uint32_t time;
} pbdrv_legodev_pup_uart_mode_switch_t;

typedef struct {
    /** The data to be set. */
    uint8_t bin_data[PBDRV_LEGODEV_MAX_DATA_SIZE]  __attribute__((aligned(4)));
    /** The size of the data to be set, also acts as set request flag. */
    uint8_t size;
    /** The mode at which to set data */
    uint8_t desired_mode;
    /** Time of the data set request (if size != 0) or time of completing transmission (if size == 0). */
    uint32_t time;
} pbdrv_legodev_pup_uart_data_set_t;

/**
 * struct ev3_uart_port_data - Data for EV3/LPF2 UART Sensor communication
 */
struct _pbdrv_legodev_pup_uart_dev_t {
    /** Main protothread, first used for synchronization thread and then for data send thread. */
    struct pt pt;
    /** Protothread for receiving sensor data, running in parallel to the data send thread. */
    struct pt recv_pt;
    /** Child protothread of the main protothread used for writing data */
    struct pt write_pt;
    /** Timer for sending keepalive messages and other delays. */
    struct etimer timer;
    /** Device information, including mode info */
    pbdrv_legodev_info_t device_info;
    /** Pointer to the UART device to use for communications. */
    pbdrv_uart_dev_t *uart;
    /** Pointer to the DC motor device to use for powered devices. */
    pbio_dcmotor_t *dcmotor;
    /**
     * Most recent binary data read from the device. How to interpret this data
     * is determined by the ::pbdrv_legodev_mode_info_t info associated with the current
     * *mode* of the device. For example, it could be an array of int32_t and/or
     * the values could be foreign-endian.
     */
    uint8_t *bin_data;
    /** The current device connection state. */
    pbdrv_legodev_pup_uart_status_t status;
    /** True if this is a dummy device. */
    bool is_dummy;
    /** Mode switch status. */
    pbdrv_legodev_pup_uart_mode_switch_t mode_switch;
    /** Data set buffer and status. */
    pbdrv_legodev_pup_uart_data_set_t *data_set;
    /** Extra mode adder for Powered Up devices (for modes > LUMP_MAX_MODE). */
    uint8_t ext_mode;
    /** New baud rate that will be set with ev3_uart_change_bitrate. */
    uint32_t new_baud_rate;
    /** Buffer to hold messages transmitted to the device. */
    uint8_t *tx_msg;
    /** Size of the current message being transmitted. */
    uint8_t tx_msg_size;
    /** Buffer to hold messages received from the device. */
    uint8_t *rx_msg;
    /** Size of the current message being received. */
    uint8_t rx_msg_size;
    /** Total number of errors that have occurred. */
    uint32_t err_count;
    /** Number of bad reads when receiving DATA ludev->msgs. */
    uint32_t num_data_err;
    /** Time of most recently started transmission. */
    uint32_t tx_start_time;
    /** Flag that indicates that good DATA ludev->msg has been received since last watchdog timeout. */
    bool data_rec;
    /** Return value for synchronization thread. */
    pbio_error_t err;
    /** ludev->msg to be printed in case of an error. */
    DBG_ERR(const char *last_err);
    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    /** Mode value used to keep track of mode in INFO messages while syncing. */
    uint8_t new_mode;
    /** Flags indicating what information has already been read from the data. */
    uint32_t info_flags;
    #endif // #define PBDRV_CONFIG_LEGODEV_MODE_INFO
};

enum {
    BUF_TX_MSG,
    BUF_RX_MSG,
    NUM_BUF
};

static uint8_t bufs[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV][NUM_BUF][EV3_UART_MAX_MESSAGE_SIZE];

static pbdrv_legodev_pup_uart_dev_t ludevs[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV];

// The following data is really just part of ludevs, but separate allocation reduces overal code size
static uint8_t data_read_bufs[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV][PBDRV_LEGODEV_MAX_DATA_SIZE] __attribute__((aligned(4)));
static pbdrv_legodev_pup_uart_data_set_t data_set_bufs[PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV];

#define PBIO_PT_WAIT_READY(pt, expr) PT_WAIT_UNTIL((pt), (expr) != PBIO_ERROR_AGAIN)

// Set up dummy motor info
void pbdrv_legodev_pup_uart_set_dummy_info(pbdrv_legodev_pup_uart_dev_t *ludev, pbdrv_legodev_type_id_t type_id) {
    ludev->is_dummy = true;
    ludev->device_info.type_id = type_id;
    ludev->device_info.flags = pbdrv_legodev_spec_basic_flags(type_id);

    uint8_t mode = pbdrv_legodev_spec_default_mode(type_id);
    ludev->device_info.mode = mode;
    ludev->mode_switch.desired_mode = mode;
    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_DATA;
    ludev->mode_switch.time = 0;
    ludev->data_set->time = 0;
    ludev->data_set->size = 0;
    ludev->device_info.num_modes = 3;
    ludev->device_info.mode_info[0] = (pbdrv_legodev_mode_info_t) {
        .num_values = 1,
        .data_type = PBDRV_LEGODEV_DATA_TYPE_INT8,
        .writable = true,
    };
    ludev->device_info.mode_info[1] = (pbdrv_legodev_mode_info_t) {
        .num_values = 1,
        .data_type = PBDRV_LEGODEV_DATA_TYPE_INT8,
        .writable = true,
    };
    ludev->device_info.mode_info[2] = (pbdrv_legodev_mode_info_t) {
        .num_values = 1,
        .data_type = PBDRV_LEGODEV_DATA_TYPE_INT32,
        .writable = false,
    };
}

pbdrv_legodev_pup_uart_dev_t *pbdrv_legodev_pup_uart_configure(uint8_t device_index, uint8_t uart_driver_index, pbio_dcmotor_t *dcmotor) {
    pbdrv_legodev_pup_uart_dev_t *ludev = &ludevs[device_index];
    ludev->dcmotor = dcmotor;
    ludev->tx_msg = &bufs[device_index][BUF_TX_MSG][0];
    ludev->rx_msg = &bufs[device_index][BUF_RX_MSG][0];
    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_ERR;
    ludev->is_dummy = false; // AI added this. Theoretically happens when a motor is plugged in after a program has already been running with a dummy motor initialized. Untested.
    ludev->err_count = 0;
    ludev->data_set = &data_set_bufs[device_index];
    ludev->bin_data = data_read_bufs[device_index];

    // legodev driver is started after all other drivers, so we
    // assume that we do not need to wait for this to be ready.
    pbdrv_uart_get(uart_driver_index, &ludev->uart);
    return ludev;
}

static void pbdrv_legodev_request_mode(pbdrv_legodev_pup_uart_dev_t *ludev, uint8_t mode) {
    ludev->mode_switch.desired_mode = mode;
    ludev->mode_switch.time = pbdrv_clock_get_ms();
    ludev->mode_switch.requested = true;
    pbdrv_legodev_pup_uart_process_poll();
}

static void pbdrv_legodev_request_data_set(pbdrv_legodev_pup_uart_dev_t *ludev, uint8_t mode, const uint8_t *data, uint8_t size) {
    ludev->data_set->size = size;
    ludev->data_set->desired_mode = mode;
    ludev->data_set->time = pbdrv_clock_get_ms();
    memcpy(ludev->data_set->bin_data, data, size);
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


static void pbdrv_legodev_pup_uart_parse_msg(pbdrv_legodev_pup_uart_dev_t *ludev) {
    uint32_t speed;
    uint8_t msg_type, cmd, msg_size, mode, cmd2;

    msg_type = ludev->rx_msg[0] & LUMP_MSG_TYPE_MASK;
    cmd = ludev->rx_msg[0] & LUMP_MSG_CMD_MASK;
    msg_size = ev3_uart_get_msg_size(ludev->rx_msg[0]);
    mode = cmd;
    cmd2 = ludev->rx_msg[1];

    // The original EV3 spec only allowed for up to 8 modes (3-bit number).
    // The Powered UP spec extents this by adding an extra flag to INFO commands.
    // Not sure that LUMP_INFO_MODE_PLUS_8 is used in practice, but rather
    // an extra (separate) LUMP_CMD_EXT_MODE message seems to be used instead
    if (msg_type == LUMP_MSG_TYPE_INFO && (cmd2 & LUMP_INFO_MODE_PLUS_8)) {
        mode += 8;
        cmd2 &= ~LUMP_INFO_MODE_PLUS_8;
    } else {
        mode += ludev->ext_mode;
    }

    if (msg_size > 1) {
        uint8_t checksum = 0xFF;
        for (int i = 0; i < msg_size - 1; i++) {
            checksum ^= ludev->rx_msg[i];
        }
        if (checksum != ludev->rx_msg[msg_size - 1]) {
            DBG_ERR(ludev->last_err = "Bad checksum");
            // if INFO messages are done and we are now receiving data, it is
            // OK to occasionally have a bad checksum
            if (ludev->status == PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {

                // The LEGO EV3 color sensor sends bad checksums
                // for RGB-RAW data (mode 4). The check here could be
                // improved if someone can find a pattern.
                if (ludev->device_info.type_id != PBDRV_LEGODEV_TYPE_ID_EV3_COLOR_SENSOR
                    || ludev->rx_msg[0] != (LUMP_MSG_TYPE_DATA | LUMP_MSG_SIZE_8 | 4)) {
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
                    if (!ludev->device_info.num_modes) {
                        DBG_ERR(ludev->last_err = "Received ACK before all mode INFO");
                        goto err;
                    }
                    if ((ludev->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                        DBG_ERR(ludev->last_err = "Did not receive all required INFO");
                        goto err;
                    }
                    ludev->device_info.mode = ludev->new_mode;
                    #endif

                    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_ACK;

                    return;
            }
            break;
        case LUMP_MSG_TYPE_CMD:
            switch (cmd) {
                case LUMP_CMD_MODES:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_MODES, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate modes INFO");
                        goto err;
                    }
                    if (cmd2 > LUMP_MAX_MODE) {
                        DBG_ERR(ludev->last_err = "Number of modes is out of range");
                        goto err;
                    }
                    ludev->device_info.num_modes = cmd2 + 1;
                    if (msg_size > 5) {
                        // Powered Up devices can have an extended mode message that
                        // includes modes > LUMP_MAX_MODE
                        ludev->device_info.num_modes = ludev->rx_msg[3] + 1;
                    }

                    debug_pr("num_modes: %d\n", ludev->device_info.num_modes);
                    #endif
                    break;
                case LUMP_CMD_SPEED:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_SPEED, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate speed INFO");
                        goto err;
                    }
                    #endif
                    speed = pbio_get_uint32_le(ludev->rx_msg + 1);
                    if (speed < EV3_UART_SPEED_MIN || speed > EV3_UART_SPEED_MAX) {
                        DBG_ERR(ludev->last_err = "Speed is out of range");
                        goto err;
                    }
                    ludev->new_baud_rate = speed;

                    debug_pr("speed: %" PRIu32 "\n", speed);

                    break;
                case LUMP_CMD_WRITE:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (cmd2 & 0x20) {
                        // TODO: write_cmd_size = cmd2 & 0x3;
                        if (ludev->info_flags & PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS) {
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
                    ludev->ext_mode = ludev->rx_msg[1];
                    break;
                case LUMP_CMD_VERSION:
                    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_VERSION, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate version INFO");
                        goto err;
                    }
                    // TODO: this might be useful someday
                    debug_pr("fw version: %08" PRIx32 "\n", pbio_get_uint32_le(ludev->rx_msg + 1));
                    debug_pr("hw version: %08" PRIx32 "\n", pbio_get_uint32_le(ludev->rx_msg + 5));
                    #endif // LUMP_CMD_VERSION
                    break;
                default:
                    DBG_ERR(ludev->last_err = "Unknown command");
                    goto err;
            }
            break;
        #if PBDRV_CONFIG_LEGODEV_MODE_INFO
        case LUMP_MSG_TYPE_INFO:
            switch (cmd2) {
                case LUMP_INFO_NAME: {
                    ludev->info_flags &= ~EV3_UART_INFO_FLAG_ALL_INFO;
                    if (ludev->rx_msg[2] < 'A' || ludev->rx_msg[2] > 'z') {
                        DBG_ERR(ludev->last_err = "Invalid name INFO");
                        goto err;
                    }
                    /*
                    * Name may not have null terminator and we
                    * are done with the checksum at this point
                    * so we are writing 0 over the checksum to
                    * ensure a null terminator for the string
                    * functions.
                    */
                    ludev->rx_msg[msg_size - 1] = 0;
                    const char *name = (char *)(ludev->rx_msg + 2);
                    size_t name_len = strlen(name);
                    if (name_len > LUMP_MAX_NAME_SIZE) {
                        DBG_ERR(ludev->last_err = "Name is too long");
                        goto err;
                    }
                    ludev->new_mode = mode;
                    ludev->info_flags |= EV3_UART_INFO_FLAG_INFO_NAME;
                    strncpy(ludev->device_info.mode_info[mode].name, name, name_len);

                    // newer LEGO UART devices send additional 6 mode capability flags
                    uint8_t flags = 0;
                    if (name_len <= LUMP_MAX_SHORT_NAME_SIZE && msg_size > LUMP_MAX_NAME_SIZE) {
                        // Only the first is used in practice.
                        flags = ludev->rx_msg[8];
                    } else {
                        // for newer devices that don't send it, set flags by device ID
                        // TODO: Look up from static info like we do for basic devices
                        if (ludev->device_info.type_id == PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR) {
                            flags = LUMP_MODE_FLAGS0_MOTOR | LUMP_MODE_FLAGS0_MOTOR_POWER | LUMP_MODE_FLAGS0_MOTOR_SPEED | LUMP_MODE_FLAGS0_MOTOR_REL_POS;
                        }
                    }

                    // Although capabilities are sent per mode, we apply them to the whole device
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_POWER) {
                        ludev->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_IS_DC_OUTPUT;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_SPEED) {
                        ludev->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_SPEED;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_REL_POS) {
                        ludev->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS;
                    }
                    if (flags & LUMP_MODE_FLAGS0_MOTOR_ABS_POS) {
                        ludev->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS;
                    }
                    if (flags & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN1) {
                        ludev->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1;
                    }
                    if (flags & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN2) {
                        ludev->device_info.flags |= PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2;
                    }

                    debug_pr("new_mode: %d\n", ludev->new_mode);
                    debug_pr("flags: %02X %02X %02X %02X %02X %02X\n",
                        ludev->rx_msg[8 + 0], ludev->rx_msg[8 + 1], ludev->rx_msg[8 + 2],
                        ludev->rx_msg[8 + 3], ludev->rx_msg[8 + 4], ludev->rx_msg[8 + 5]);
                }
                break;
                case LUMP_INFO_RAW:
                case LUMP_INFO_PCT:
                case LUMP_INFO_SI:
                case LUMP_INFO_UNITS:
                    break;
                case LUMP_INFO_MAPPING:
                    if (ludev->new_mode != mode) {
                        DBG_ERR(ludev->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MAPPING, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate mapping INFO");
                        goto err;
                    }

                    // Mode supports writing if rx_msg[3] is nonzero.
                    ludev->device_info.mode_info[mode].writable = ludev->rx_msg[3] != 0;

                    debug_pr("mapping: in %02x out %02x\n", ludev->rx_msg[2], ludev->rx_msg[3]);
                    debug_pr("mapping: in %02x out %02x\n", ludev->rx_msg[2], ludev->rx_msg[3]);
                    debug_pr("Writable: %d\n", ludev->device_info.mode_info[mode].writable);

                    break;
                case LUMP_INFO_MODE_COMBOS:
                    if (ludev->new_mode != mode) {
                        DBG_ERR(ludev->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MODE_COMBOS, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate mode combos INFO");
                        goto err;
                    }

                    // REVISIT: this is potentially an array of combos
                    debug_pr("mode combos: %04x\n", ludev->rx_msg[3] << 8 | ludev->rx_msg[2]);

                    break;
                case LUMP_INFO_UNK9:
                    if (ludev->new_mode != mode) {
                        DBG_ERR(ludev->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK9, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate UNK9 INFO");
                        goto err;
                    }

                    // first 3 parameters look like PID constants, 4th is max tacho_rate
                    debug_pr("motor parameters: %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
                        pbio_get_uint32_le(ludev->rx_msg + 2), pbio_get_uint32_le(ludev->rx_msg + 6),
                        pbio_get_uint32_le(ludev->rx_msg + 10), pbio_get_uint32_le(ludev->rx_msg + 14));

                    break;
                case LUMP_INFO_UNK11:
                    if (ludev->new_mode != mode) {
                        DBG_ERR(ludev->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK11, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate UNK11 INFO");
                        goto err;
                    }
                    break;
                case LUMP_INFO_FORMAT:
                    if (ludev->new_mode != mode) {
                        DBG_ERR(ludev->last_err = "Received INFO for incorrect mode");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_FORMAT, &ludev->info_flags)) {
                        DBG_ERR(ludev->last_err = "Received duplicate format INFO");
                        goto err;
                    }
                    ludev->device_info.mode_info[mode].num_values = ludev->rx_msg[2];
                    if (!ludev->device_info.mode_info[mode].num_values) {
                        DBG_ERR(ludev->last_err = "Invalid number of data sets");
                        goto err;
                    }
                    if (msg_size < 7) {
                        DBG_ERR(ludev->last_err = "Invalid format ludev->msg size");
                        goto err;
                    }
                    if ((ludev->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                        DBG_ERR(ludev->last_err = "Did not receive all required INFO");
                        goto err;
                    }
                    ludev->device_info.mode_info[mode].data_type = ludev->rx_msg[3];
                    if (ludev->new_mode) {
                        ludev->new_mode--;
                    }

                    debug_pr("num_values: %d\n", ludev->device_info.mode_info[mode].num_values);
                    debug_pr("data_type: %d\n", ludev->device_info.mode_info[mode].data_type);

                    break;
            }
            break;
        #endif
        case LUMP_MSG_TYPE_DATA:
            if (ludev->status != PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {
                DBG_ERR(ludev->last_err = "Received DATA before INFO was complete");
                goto err;
            }

            #if PBDRV_CONFIG_LEGODEV_MODE_INFO
            if (mode >= ludev->device_info.num_modes) {
                DBG_ERR(ludev->last_err = "Invalid mode received");
                goto err;
            }
            #endif

            // Data is for requested mode.
            if (mode == ludev->mode_switch.desired_mode) {
                memcpy(ludev->bin_data, ludev->rx_msg + 1, msg_size - 2);

                if (ludev->device_info.mode != mode) {
                    // First time getting data in this mode, so register time.
                    ludev->mode_switch.time = pbdrv_clock_get_ms();
                }
            }
            ludev->device_info.mode = mode;

            ludev->data_rec = true;
            if (ludev->num_data_err) {
                ludev->num_data_err--;
            }
            break;
    }

    return;

err:
    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_ERR;
    debug_pr("Data error: %s\n", ludev->last_err);
}

static uint8_t ev3_uart_set_msg_hdr(lump_msg_type_t type, lump_msg_size_t size, lump_cmd_t cmd) {
    return (type & LUMP_MSG_TYPE_MASK) | (size & LUMP_MSG_SIZE_MASK) | (cmd & LUMP_MSG_CMD_MASK);
}

static void ev3_uart_prepare_tx_msg(pbdrv_legodev_pup_uart_dev_t *ludev, lump_msg_type_t msg_type,
    lump_cmd_t cmd, const uint8_t *data, uint8_t len) {
    uint8_t header, checksum, i;
    uint8_t offset = 0;
    lump_msg_size_t size;

    if (msg_type == LUMP_MSG_TYPE_DATA) {
        // Only Powered Up devices support setting data, and they expect to have an
        // extra command sent to give the part of the mode > 7
        ludev->tx_msg[0] = ev3_uart_set_msg_hdr(LUMP_MSG_TYPE_CMD, LUMP_MSG_SIZE_1, LUMP_CMD_EXT_MODE);
        ludev->tx_msg[1] = ludev->device_info.mode > LUMP_MAX_MODE ? 8 : 0;
        ludev->tx_msg[2] = 0xff ^ ludev->tx_msg[0] ^ ludev->tx_msg[1];
        offset = 3;
    }

    checksum = 0xff;
    for (i = 0; i < len; i++) {
        ludev->tx_msg[offset + i + 1] = data[i];
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
        ludev->tx_msg[offset + i + 1] = 0;
    }

    header = ev3_uart_set_msg_hdr(msg_type, size, cmd);
    checksum ^= header;

    ludev->tx_msg[offset] = header;
    ludev->tx_msg[offset + i + 1] = checksum;
    ludev->tx_msg_size = offset + i + 2;
}

static void pbdrv_legodev_pup_uart_reset(pbdrv_legodev_pup_uart_dev_t *ludev) {
    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_ERR;
    if (ludev->dcmotor != NULL && ludev->dcmotor->motor_driver != NULL) {
        pbdrv_motor_driver_coast(ludev->dcmotor->motor_driver);
    }
}

static PT_THREAD(pbdrv_legodev_pup_uart_send_prepared_msg(pbdrv_legodev_pup_uart_dev_t * ludev, pbio_error_t * err)) {
    PT_BEGIN(&ludev->write_pt);
    ludev->tx_start_time = pbdrv_clock_get_ms();
    PBIO_PT_WAIT_READY(&ludev->write_pt, *err = pbdrv_uart_write_begin(ludev->uart, ludev->tx_msg, ludev->tx_msg_size, EV3_UART_IO_TIMEOUT));
    if (*err == PBIO_SUCCESS) {
        PBIO_PT_WAIT_READY(&ludev->write_pt, *err = pbdrv_uart_write_end(ludev->uart));
    }
    PT_END(&ludev->write_pt);
}

/**
 * The synchronization thread for the LEGO UART device.
 *
 * This thread receives and parses incoming messages sent by LEGO UART devices
 * when they are plugged in. It populates the device state accordingly.
 *
 * @param [in]  ludev       The LEGO UART device instance.
 */
static PT_THREAD(pbdrv_legodev_pup_uart_synchronize_thread(pbdrv_legodev_pup_uart_dev_t * ludev)) {

    PT_BEGIN(&ludev->pt);

    // reset state for new device
    ludev->is_dummy = false; // AI added this. Theoretically happens when a motor is plugged in after a program has already been running with a dummy motor initialized. Untested.
    ludev->device_info.type_id = PBDRV_LEGODEV_TYPE_ID_NONE;
    ludev->device_info.mode = 0;
    ludev->ext_mode = 0;
    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    ludev->device_info.flags = PBDRV_LEGODEV_CAPABILITY_FLAG_NONE;
    #endif
    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_SYNCING;

    // Send SPEED command at 115200 baud
    debug_pr("set baud: %d\n", EV3_UART_SPEED_LPF2);
    pbdrv_uart_set_baud_rate(ludev->uart, EV3_UART_SPEED_LPF2);
    uint8_t speed_payload[4];
    pbio_set_uint32_le(speed_payload, EV3_UART_SPEED_LPF2);
    ev3_uart_prepare_tx_msg(ludev, LUMP_MSG_TYPE_CMD, LUMP_CMD_SPEED, speed_payload, sizeof(speed_payload));

    pbdrv_uart_flush(ludev->uart);

    PT_SPAWN(&ludev->pt, &ludev->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(ludev, &ludev->err));
    if (ludev->err != PBIO_SUCCESS) {
        DBG_ERR(ludev->last_err = "Speed tx failed");
        PT_EXIT(&ludev->pt);
    }

    pbdrv_uart_flush(ludev->uart);

    // read one byte to check for ACK
    PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_begin(ludev->uart, ludev->rx_msg, 1, 10));
    if (ludev->err != PBIO_SUCCESS) {
        DBG_ERR(ludev->last_err = "UART Rx error during baud");
        PT_EXIT(&ludev->pt);
    }

    PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_end(ludev->uart));
    if ((ludev->err == PBIO_SUCCESS && ludev->rx_msg[0] != LUMP_SYS_ACK) || ludev->err == PBIO_ERROR_TIMEDOUT) {
        // if we did not get ACK within 100ms, then switch to slow baud rate for sync
        pbdrv_uart_set_baud_rate(ludev->uart, EV3_UART_SPEED_MIN);
        debug_pr("set baud: %d\n", EV3_UART_SPEED_MIN);
    } else if (ludev->err != PBIO_SUCCESS) {
        DBG_ERR(ludev->last_err = "UART Rx error during baud");
        PT_EXIT(&ludev->pt);
    }

sync:

    // To get in sync with the data stream from the sensor, we look for a valid TYPE command.
    for (;;) {

        PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_begin(ludev->uart, ludev->rx_msg, 1, EV3_UART_IO_TIMEOUT));
        if (ludev->err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx error during sync");
            PT_EXIT(&ludev->pt);
        }
        PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_end(ludev->uart));
        if (ludev->err == PBIO_ERROR_TIMEDOUT) {
            continue;
        }
        if (ludev->err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx error during sync");
            PT_EXIT(&ludev->pt);
        }

        if (ludev->rx_msg[0] == (LUMP_MSG_TYPE_CMD | LUMP_CMD_TYPE)) {
            break;
        }
    }

    // then read the rest of the message

    PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_begin(ludev->uart, ludev->rx_msg + 1, 2, EV3_UART_IO_TIMEOUT));
    if (ludev->err != PBIO_SUCCESS) {
        DBG_ERR(ludev->last_err = "UART Rx error while reading type");
        PT_EXIT(&ludev->pt);
    }
    PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_end(ludev->uart));
    if (ludev->err != PBIO_SUCCESS) {
        DBG_ERR(ludev->last_err = "UART Rx error while reading type");
        PT_EXIT(&ludev->pt);
    }

    bool bad_id = ludev->rx_msg[1] < EV3_UART_TYPE_MIN || ludev->rx_msg[1] > EV3_UART_TYPE_MAX;
    uint8_t checksum = 0xff ^ ludev->rx_msg[0] ^ ludev->rx_msg[1];
    bool bad_id_checksum = ludev->rx_msg[2] != checksum;

    if (bad_id || bad_id_checksum) {
        DBG_ERR(ludev->last_err = "Bad device type id or checksum");
        if (ludev->err_count > 10) {
            ludev->err_count = 0;
            ludev->err = PBIO_ERROR_IO;
            PT_EXIT(&ludev->pt);
        }
        ludev->err_count++;
        goto sync;
    }

    // if all was good, we are ready to start receiving the mode info
    ludev->device_info.type_id = ludev->rx_msg[1];
    ludev->data_rec = false;
    ludev->num_data_err = 0;
    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_INFO;
    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    ludev->device_info.flags = PBDRV_LEGODEV_CAPABILITY_FLAG_NONE;
    ludev->info_flags = EV3_UART_INFO_FLAG_CMD_TYPE;
    ludev->device_info.num_modes = 1;
    #endif
    debug_pr("type id: %d\n", ludev->device_info.type_id);

    while (ludev->status == PBDRV_LEGODEV_PUP_UART_STATUS_INFO) {
        // read the message header
        PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_begin(ludev->uart, ludev->rx_msg, 1, EV3_UART_IO_TIMEOUT));
        if (ludev->err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx begin error during info header");
            PT_EXIT(&ludev->pt);
        }
        PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_end(ludev->uart));
        if (ludev->err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx end error during info header");
            PT_EXIT(&ludev->pt);
        }

        ludev->rx_msg_size = ev3_uart_get_msg_size(ludev->rx_msg[0]);
        if (ludev->rx_msg_size > EV3_UART_MAX_MESSAGE_SIZE) {
            DBG_ERR(ludev->last_err = "Bad message size during info");
            ludev->err = PBIO_ERROR_IO;
            PT_EXIT(&ludev->pt);
        }

        // read the rest of the message
        if (ludev->rx_msg_size > 1) {
            PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_begin(ludev->uart, ludev->rx_msg + 1, ludev->rx_msg_size - 1, EV3_UART_IO_TIMEOUT));
            if (ludev->err != PBIO_SUCCESS) {
                DBG_ERR(ludev->last_err = "UART Rx begin error during info");
                PT_EXIT(&ludev->pt);
            }
            PBIO_PT_WAIT_READY(&ludev->pt, ludev->err = pbdrv_uart_read_end(ludev->uart));
            if (ludev->err != PBIO_SUCCESS) {
                DBG_ERR(ludev->last_err = "UART Rx end error during info");
                PT_EXIT(&ludev->pt);
            }
        }

        // at this point, we have a full ludev->msg that can be parsed
        pbdrv_legodev_pup_uart_parse_msg(ludev);
    }

    // at this point we should have read all of the mode info
    if (ludev->status != PBDRV_LEGODEV_PUP_UART_STATUS_ACK) {
        // ludev->last_err should be set by pbdrv_legodev_pup_uart_parse_msg()
        ludev->err = PBIO_ERROR_IO;
        PT_EXIT(&ludev->pt);
    }

    // reply with ACK
    ludev->tx_msg[0] = LUMP_SYS_ACK;
    ludev->tx_msg_size = 1;
    PT_SPAWN(&ludev->pt, &ludev->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(ludev, &ludev->err));
    if (ludev->err != PBIO_SUCCESS) {
        DBG_ERR(ludev->last_err = "UART Tx error during ack.");
        PT_EXIT(&ludev->pt);
    }

    // schedule baud rate change
    etimer_set(&ludev->timer, 10);
    PT_WAIT_UNTIL(&ludev->pt, etimer_expired(&ludev->timer));

    // change the baud rate
    pbdrv_uart_set_baud_rate(ludev->uart, ludev->new_baud_rate);
    debug_pr("set baud: %" PRIu32 "\n", ludev->new_baud_rate);

    // Load static flags on platforms that don't read device info
    #if !PBDRV_CONFIG_LEGODEV_MODE_INFO
    ludev->device_info.flags = pbdrv_legodev_spec_basic_flags(ludev->device_info.type_id);
    #endif

    // Turn on power for devices that need it
    if (ludev->device_info.flags & PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1) {
        pbdrv_motor_driver_set_duty_cycle(ludev->dcmotor->motor_driver, -PBDRV_MOTOR_DRIVER_MAX_DUTY);
    } else if (ludev->device_info.flags & PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2) {
        pbdrv_motor_driver_set_duty_cycle(ludev->dcmotor->motor_driver, PBDRV_MOTOR_DRIVER_MAX_DUTY);
    } else {
        pbdrv_motor_driver_coast(ludev->dcmotor->motor_driver);
    }

    // Request switch to default mode for this device.
    pbdrv_legodev_request_mode(ludev, pbdrv_legodev_spec_default_mode(ludev->device_info.type_id));

    // Reset other timers
    etimer_reset_with_new_interval(&ludev->timer, EV3_UART_DATA_KEEP_ALIVE_TIMEOUT);
    ludev->data_set->time = pbdrv_clock_get_ms();
    ludev->data_set->size = 0;

    ludev->status = PBDRV_LEGODEV_PUP_UART_STATUS_DATA;

    PT_END(&ludev->pt);
}

/**
 * The send thread for the LEGO UART device.
 *
 * This thread periodically sends a keep-alive message to the device. It also
 * sends any data or mode changes that have been queued for transmission.
 *
 * @param [in]  ludev       The LEGO UART device instance.
 */
static PT_THREAD(pbdrv_legodev_pup_uart_send_thread(pbdrv_legodev_pup_uart_dev_t * ludev)) {

    PT_BEGIN(&ludev->pt);

    while (ludev->status == PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {

        PT_WAIT_UNTIL(&ludev->pt, etimer_expired(&ludev->timer) || ludev->mode_switch.requested || ludev->data_set->size > 0);

        // Handle keep alive timeout
        if (etimer_expired(&ludev->timer)) {
            // make sure we are receiving data
            if (!ludev->data_rec) {
                ludev->num_data_err++;
                DBG_ERR(ludev->last_err = "No data since last keepalive");
                if (ludev->num_data_err > 6) {
                    ludev->err = PBIO_ERROR_IO;
                    PT_EXIT(&ludev->pt);
                }
            }
            ludev->data_rec = false;
            ludev->tx_msg[0] = LUMP_SYS_NACK;
            ludev->tx_msg_size = 1;
            PT_SPAWN(&ludev->pt, &ludev->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(ludev, &ludev->err));
            if (ludev->err != PBIO_SUCCESS) {
                DBG_ERR(ludev->last_err = "Error during keepalive.");
                PT_EXIT(&ludev->pt);
            }
            etimer_reset_with_new_interval(&ludev->timer, EV3_UART_DATA_KEEP_ALIVE_TIMEOUT);

            // Retry mode switch if it hasn't been handled or failed.
            if (ludev->device_info.mode != ludev->mode_switch.desired_mode && pbdrv_clock_get_ms() - ludev->mode_switch.time > EV3_UART_IO_TIMEOUT) {
                ludev->mode_switch.requested = true;
            }
        }

        // Handle requested mode change
        if (ludev->mode_switch.requested) {
            ludev->mode_switch.requested = false;
            ev3_uart_prepare_tx_msg(ludev, LUMP_MSG_TYPE_CMD, LUMP_CMD_SELECT, &ludev->mode_switch.desired_mode, 1);
            PT_SPAWN(&ludev->pt, &ludev->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(ludev, &ludev->err));
            if (ludev->err != PBIO_SUCCESS) {
                DBG_ERR(ludev->last_err = "Setting requested mode failed.");
                PT_EXIT(&ludev->pt);
            }
        }

        // Handle requested data set
        if (ludev->data_set->size > 0) {
            // Only set data if we are in the correct mode already.
            if (ludev->device_info.mode == ludev->data_set->desired_mode) {
                ev3_uart_prepare_tx_msg(ludev, LUMP_MSG_TYPE_DATA, ludev->data_set->desired_mode, ludev->data_set->bin_data, ludev->data_set->size);
                ludev->data_set->size = 0;
                ludev->data_set->time = pbdrv_clock_get_ms();
                PT_SPAWN(&ludev->pt, &ludev->write_pt, pbdrv_legodev_pup_uart_send_prepared_msg(ludev, &ludev->err));
                if (ludev->err != PBIO_SUCCESS) {
                    DBG_ERR(ludev->last_err = "Setting requested data failed.");
                    PT_EXIT(&ludev->pt);
                }
                ludev->data_set->time = pbdrv_clock_get_ms();
            } else if (pbdrv_clock_get_ms() - ludev->data_set->time < 500) {
                // Not in the right mode yet, try again later for a reasonable amount of time.
                pbdrv_legodev_pup_uart_process_poll();
                PT_YIELD(&ludev->pt);
            } else {
                // Give up setting data.
                ludev->data_set->size = 0;
            }
        }
    }

    PT_END(&ludev->pt);
}

/**
 * The receive thread for the LEGO UART device.
 *
 * @param [in]  ludev       The LEGO UART device instance.
 */
static PT_THREAD(pbdrv_legodev_pup_uart_receive_data_thread(pbdrv_legodev_pup_uart_dev_t * ludev)) {

    // REVISIT: This is not the greatest. We can easily get a buffer overrun and
    // loose data. For now, the retry after bad message size helps get back into
    // sync with the data stream.

    pbio_error_t err;

    PT_BEGIN(&ludev->recv_pt);

    while (true) {
        PBIO_PT_WAIT_READY(&ludev->recv_pt,
            err = pbdrv_uart_read_begin(ludev->uart, ludev->rx_msg, 1, EV3_UART_IO_TIMEOUT));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx data header begin error");
            break;
        }
        PBIO_PT_WAIT_READY(&ludev->recv_pt, err = pbdrv_uart_read_end(ludev->uart));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx data header end error");
            break;
        }

        ludev->rx_msg_size = ev3_uart_get_msg_size(ludev->rx_msg[0]);
        if (ludev->rx_msg_size < 3 || ludev->rx_msg_size > EV3_UART_MAX_MESSAGE_SIZE) {
            DBG_ERR(ludev->last_err = "Bad data message size");
            continue;
        }

        uint8_t msg_type = ludev->rx_msg[0] & LUMP_MSG_TYPE_MASK;
        uint8_t cmd = ludev->rx_msg[0] & LUMP_MSG_CMD_MASK;
        if (msg_type != LUMP_MSG_TYPE_DATA && (msg_type != LUMP_MSG_TYPE_CMD ||
                                               (cmd != LUMP_CMD_WRITE && cmd != LUMP_CMD_EXT_MODE))) {
            DBG_ERR(ludev->last_err = "Bad msg type");
            continue;
        }

        PBIO_PT_WAIT_READY(&ludev->recv_pt,
            err = pbdrv_uart_read_begin(ludev->uart, ludev->rx_msg + 1, ludev->rx_msg_size - 1, EV3_UART_IO_TIMEOUT));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx data begin error");
            break;
        }
        PBIO_PT_WAIT_READY(&ludev->recv_pt, err = pbdrv_uart_read_end(ludev->uart));
        if (err != PBIO_SUCCESS) {
            DBG_ERR(ludev->last_err = "UART Rx data end error");
            break;
        }

        // at this point, we have a full ludev->msg that can be parsed
        pbdrv_legodev_pup_uart_parse_msg(ludev);
    }

    PT_END(&ludev->recv_pt);
}

/**
 * The main thread for the LEGO UART device. This drives the synchronization
 * and then the data send/receive threads.
 *
 * This thread can be spawned once a device is detected, or spawned repeatedly
 * if the platform does not support automatic device detection.
 *
 * @param [in]  pt          Protothread to run main thread on.
 * @param [in]  ludev       The LEGO UART device instance.
 */
PT_THREAD(pbdrv_legodev_pup_uart_thread(struct pt *pt, pbdrv_legodev_pup_uart_dev_t *ludev)) {
    PT_BEGIN(pt);

    // Get in in sync with the sensor information dump and parse it.
    PT_SPAWN(pt, &ludev->pt, pbdrv_legodev_pup_uart_synchronize_thread(ludev));
    if (ludev->err != PBIO_SUCCESS) {
        pbdrv_legodev_pup_uart_reset(ludev);
        PT_EXIT(pt);
    }

    // The sensor is now ready for use. Now run the send and receive threads in
    // parallel until the send thread ends or exits.
    PT_INIT(&ludev->pt);
    PT_INIT(&ludev->recv_pt);
    while (PT_SCHEDULE(pbdrv_legodev_pup_uart_send_thread(ludev))) {
        pbdrv_legodev_pup_uart_receive_data_thread(ludev);
        PT_YIELD(pt);
    }
    pbdrv_legodev_pup_uart_reset(ludev);

    PT_END(pt);
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

/**
 * Checks if LEGO UART device has data available for reading or is ready to write.
 *
 * @param [in]  legodev     The legodev instance.
 * @return                  ::PBIO_SUCCESS if ready.
 *                          ::PBIO_ERROR_AGAIN if not ready yet.
 *                          ::PBIO_ERROR_NO_DEV if no device is attached.
 */
pbio_error_t pbdrv_legodev_is_ready(pbdrv_legodev_dev_t *legodev) {

    pbdrv_legodev_pup_uart_dev_t *ludev = pbdrv_legodev_get_uart_dev(legodev);
    if (!ludev || ludev->status == PBDRV_LEGODEV_PUP_UART_STATUS_ERR) {
        return PBIO_ERROR_NO_DEV;
    }

    // For dummy devices, skip the rest of the checks
    if (ludev->is_dummy) {
        return PBIO_SUCCESS;
    }

    if (ludev->status != PBDRV_LEGODEV_PUP_UART_STATUS_DATA) {
        return PBIO_ERROR_AGAIN;
    }

    uint32_t time = pbdrv_clock_get_ms();

    // Not ready if waiting for mode change
    if (ludev->device_info.mode != ludev->mode_switch.desired_mode) {
        return PBIO_ERROR_AGAIN;
    }

    // Not ready if waiting for stale data to be discarded.
    if (time - ludev->mode_switch.time <= pbdrv_legodev_spec_stale_data_delay(ludev->device_info.type_id, ludev->device_info.mode)) {
        return PBIO_ERROR_AGAIN;
    }

    // Not ready if just recently set new data.
    if (ludev->data_set->size > 0 || time - ludev->data_set->time <= pbdrv_legodev_spec_data_set_delay(ludev->device_info.type_id, ludev->device_info.mode)) {
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

    pbdrv_legodev_pup_uart_dev_t *ludev = pbdrv_legodev_get_uart_dev(legodev);
    if (!ludev) {
        return PBIO_ERROR_NO_DEV;
    }

    // For dummy devices, set the mode directly.
    if (ludev->is_dummy) {
        ludev->device_info.mode = mode;
        ludev->mode_switch.desired_mode = mode;
        return PBIO_SUCCESS;
    }

    // Mode already set or being set, so return success.
    if (ludev->mode_switch.desired_mode == mode || ludev->device_info.mode == mode) {
        return PBIO_SUCCESS;
    }

    // We can only initiate a mode switch if currently idle (receiving data).
    pbio_error_t err = pbdrv_legodev_is_ready(legodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    // Can only set available modes.
    if (mode >= ludev->device_info.num_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }
    #endif

    // Request mode switch.
    pbdrv_legodev_request_mode(ludev, mode);

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

    pbdrv_legodev_pup_uart_dev_t *ludev = pbdrv_legodev_get_uart_dev(legodev);
    if (!ludev) {
        return PBIO_ERROR_NO_DEV;
    }

    // For dummy devices, get the mode directly.
    if (ludev->is_dummy) {
        ludev->device_info.mode = mode;
    }

    // Can only request data for mode that is set.
    if (mode != ludev->device_info.mode) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Can only read if ready.
    *data = ludev->bin_data;
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

    pbdrv_legodev_pup_uart_dev_t *ludev = pbdrv_legodev_get_uart_dev(legodev);
    if (!ludev) {
        return PBIO_ERROR_NO_DEV;
    }

    #if PBDRV_CONFIG_LEGODEV_MODE_INFO
    const pbdrv_legodev_mode_info_t *mode_info = &ludev->device_info.mode_info[mode];
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
    pbdrv_legodev_request_data_set(ludev, mode, data, size);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_info(pbdrv_legodev_dev_t *legodev, pbdrv_legodev_info_t **info) {

    pbdrv_legodev_pup_uart_dev_t *ludev = pbdrv_legodev_get_uart_dev(legodev);
    if (!ludev) {
        return PBIO_ERROR_NO_DEV;
    }
    // Info is set even in case of error. Caller can decide what values apply
    // based on the error code.
    *info = &ludev->device_info;
    return pbdrv_legodev_is_ready(legodev);
}

#endif // PBDRV_CONFIG_LEGODEV_PUP_UART

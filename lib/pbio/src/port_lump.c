// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_PORT && PBIO_CONFIG_PORT_NUM_LUMP > 0

#include <string.h>

#include <contiki.h>

#include <pbio/util.h>

#include <pbio/port_interface.h>
#include <pbio/port_lump.h>
#include <pbdrv/ioport.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define debug_pr pbdrv_uart_debug_printf
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

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
    PBDRV_LEGODEV_LUMP_STATUS_ERR,
    /** Waiting for data that looks like LEGO UART protocol. */
    PBDRV_LEGODEV_LUMP_STATUS_SYNCING,
    /** Reading device info before changing baud rate. */
    PBDRV_LEGODEV_LUMP_STATUS_INFO,
    /** ACK received, delay changing baud rate. */
    PBDRV_LEGODEV_LUMP_STATUS_ACK,
    /** Ready to send commands and receive data. */
    PBDRV_LEGODEV_LUMP_STATUS_DATA,
} pbdrv_legodev_lump_status_t;

typedef struct {
    /** The mode to be set. */
    uint8_t desired_mode;
    /** Whether a mode change was requested (set low when handled). */
    bool requested;
    /** Time of switch completion (if info.mode == desired_mode) or time of switch request (if info.mode != desired_mode). */
    uint32_t time;
} pbdrv_legodev_lump_mode_switch_t;

typedef struct {
    /** The data to be set. */
    uint8_t bin_data[LUMP_MAX_MSG_SIZE]  __attribute__((aligned(4)));
    /** The size of the data to be set, also acts as set request flag. */
    uint8_t size;
    /** The mode at which to set data */
    uint8_t desired_mode;
    /** Time of the data set request (if size != 0) or time of completing transmission (if size == 0). */
    uint32_t time;
} pbdrv_legodev_lump_data_set_t;


// LUMP state for each port.
struct _pbio_port_lump_dev_t {
    /** Parent port instance. */
    pbio_port_t *port;
    /** Child protothread of the main protothread used for reading data */
    struct pt read_pt;
    /** Child protothread of the main protothread used for writing data */
    struct pt write_pt;
    /** Timer for sending keepalive messages and other delays. */
    struct etimer timer;
    /** The current mode of the device */
    uint8_t mode;
    /** Parsed device information, including mode info */
    pbio_port_lump_device_info_t parsed_info;
    /**
     * Most recent binary data read from the device. How to interpret this data
     * is determined by the ::pbio_port_lump_mode_info_t info associated with the current
     * *mode* of the device. For example, it could be an array of int32_t and/or
     * the values could be foreign-endian.
     */
    uint8_t *bin_data;
    /** The current device connection state. */
    pbdrv_legodev_lump_status_t status;
    /** Mode switch status. */
    pbdrv_legodev_lump_mode_switch_t mode_switch;
    /** Data set buffer and status. */
    pbdrv_legodev_lump_data_set_t *data_set;
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
    /** Number of bad reads when receiving DATA lump_dev->msgs. */
    uint32_t num_data_err;
    /** Flag that indicates that good DATA lump_dev->msg has been received since last watchdog timeout. */
    bool data_rec;
    /** Return value for synchronization thread. */
    pbio_error_t err;
    /** lump_dev->msg to be printed in case of an error. */
    DBG_ERR(const char *last_err);
    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
    /** Mode value used to keep track of mode in INFO messages while syncing. */
    uint8_t new_mode;
    /** Flags indicating what information has already been read from the data. */
    uint32_t info_flags;
    #endif // PBIO_CONFIG_PORT_LUMP_MODE_INFO
};

pbio_port_lump_dev_t lump_devices[PBIO_CONFIG_PORT_NUM_LUMP];

enum {
    BUF_TX_MSG,
    BUF_RX_MSG,
    NUM_BUF
};

static uint8_t bufs[PBIO_CONFIG_PORT_NUM_LUMP][NUM_BUF][EV3_UART_MAX_MESSAGE_SIZE];

// The following data is really just part of lump_devices, but separate allocation reduces overal code size
static uint8_t data_read_bufs[PBIO_CONFIG_PORT_NUM_LUMP][LUMP_MAX_MSG_SIZE] __attribute__((aligned(4)));
static pbdrv_legodev_lump_data_set_t data_set_bufs[PBIO_CONFIG_PORT_NUM_LUMP];

pbio_port_lump_dev_t *pbio_port_lump_init_instance(uint8_t device_index, pbio_port_t *port) {
    if (device_index >= PBIO_CONFIG_PORT_NUM_LUMP) {
        return NULL;
    }
    pbio_port_lump_dev_t *lump_dev = &lump_devices[device_index];
    lump_dev->port = port;
    lump_dev->tx_msg = &bufs[device_index][BUF_TX_MSG][0];
    lump_dev->rx_msg = &bufs[device_index][BUF_RX_MSG][0];
    lump_dev->status = PBDRV_LEGODEV_LUMP_STATUS_ERR;
    lump_dev->err_count = 0;
    lump_dev->data_set = &data_set_bufs[device_index];
    lump_dev->bin_data = data_read_bufs[device_index];
    return lump_dev;
}

static void pbio_port_lump_request_mode(pbio_port_lump_dev_t *lump_dev, uint8_t mode) {
    lump_dev->mode_switch.desired_mode = mode;
    lump_dev->mode_switch.time = pbdrv_clock_get_ms();
    lump_dev->mode_switch.requested = true;
    pbio_port_process_poll(lump_dev->port);
}

static void pbio_port_lump_request_data_set(pbio_port_lump_dev_t *lump_dev, uint8_t mode, const uint8_t *data, uint8_t size) {
    lump_dev->data_set->size = size;
    lump_dev->data_set->desired_mode = mode;
    lump_dev->data_set->time = pbdrv_clock_get_ms();
    memcpy(lump_dev->data_set->bin_data, data, size);
    pbio_port_process_poll(lump_dev->port);
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

/**
 * Handles incoming motor position data.
 *
 * @param [in] lump_dev The lump device with new data.
 */
static void pbio_port_lump_handle_known_data(pbio_port_lump_dev_t *lump_dev) {
    if (lump_dev->parsed_info.capabilities & LUMP_MODE_FLAGS0_MOTOR_ABS_POS &&
        lump_dev->mode == LEGO_DEVICE_MODE_PUP_ABS_MOTOR__CALIB) {
        int32_t deci_deg = (int16_t)pbio_get_uint16_le(lump_dev->bin_data + 2);
        pbio_port_update_angle_abs_mdeg(lump_dev->port, deci_deg * 100);
        return;
    }

    if (lump_dev->parsed_info.type_id == LEGO_DEVICE_TYPE_ID_INTERACTIVE_MOTOR &&
        lump_dev->mode == LEGO_DEVICE_MODE_PUP_REL_MOTOR__POS) {
        int32_t degrees = pbio_get_uint32_le(lump_dev->bin_data);
        pbio_port_update_angle_rel_deg(lump_dev->port, degrees);
    }
}


static void pbio_port_lump_lump_parse_msg(pbio_port_lump_dev_t *lump_dev) {
    uint32_t speed;
    uint8_t msg_type, cmd, msg_size, mode, cmd2;

    msg_type = lump_dev->rx_msg[0] & LUMP_MSG_TYPE_MASK;
    cmd = lump_dev->rx_msg[0] & LUMP_MSG_CMD_MASK;
    msg_size = ev3_uart_get_msg_size(lump_dev->rx_msg[0]);
    mode = cmd;
    cmd2 = lump_dev->rx_msg[1];

    // The original EV3 spec only allowed for up to 8 modes (3-bit number).
    // The Powered UP spec extents this by adding an extra flag to INFO commands.
    // Not sure that LUMP_INFO_MODE_PLUS_8 is used in practice, but rather
    // an extra (separate) LUMP_CMD_EXT_MODE message seems to be used instead
    if (msg_type == LUMP_MSG_TYPE_INFO && (cmd2 & LUMP_INFO_MODE_PLUS_8)) {
        mode += 8;
        cmd2 &= ~LUMP_INFO_MODE_PLUS_8;
    } else {
        mode += lump_dev->ext_mode;
    }

    if (msg_size > 1) {
        uint8_t checksum = 0xFF;
        for (int i = 0; i < msg_size - 1; i++) {
            checksum ^= lump_dev->rx_msg[i];
        }
        if (checksum != lump_dev->rx_msg[msg_size - 1]) {
            debug_pr("Bad checksum\n");
            // if INFO messages are done and we are now receiving data, it is
            // OK to occasionally have a bad checksum
            if (lump_dev->status == PBDRV_LEGODEV_LUMP_STATUS_DATA) {

                // The LEGO EV3 color sensor sends bad checksums
                // for RGB-RAW data (mode 4). The check here could be
                // improved if someone can find a pattern.
                if (lump_dev->parsed_info.type_id != LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR
                    || lump_dev->rx_msg[0] != (LUMP_MSG_TYPE_DATA | LUMP_MSG_SIZE_8 | 4)) {
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
                    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
                    if (!lump_dev->parsed_info.num_modes) {
                        debug_pr("Received ACK before all mode INFO\n");
                        goto err;
                    }
                    if ((lump_dev->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                        debug_pr("Did not receive all required INFO\n");
                        goto err;
                    }
                    lump_dev->mode = lump_dev->new_mode;
                    #endif

                    lump_dev->status = PBDRV_LEGODEV_LUMP_STATUS_ACK;

                    return;
            }
            break;
        case LUMP_MSG_TYPE_CMD:
            switch (cmd) {
                case LUMP_CMD_MODES:
                    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_MODES, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate modes INFO\n");
                        goto err;
                    }
                    if (cmd2 > LUMP_MAX_MODE) {
                        debug_pr("Number of modes is out of range\n");
                        goto err;
                    }
                    lump_dev->parsed_info.num_modes = cmd2 + 1;
                    if (msg_size > 5) {
                        // Powered Up devices can have an extended mode message that
                        // includes modes > LUMP_MAX_MODE
                        lump_dev->parsed_info.num_modes = lump_dev->rx_msg[3] + 1;
                    }

                    debug_pr("num_modes: %d\n", lump_dev->parsed_info.num_modes);
                    #endif
                    break;
                case LUMP_CMD_SPEED:
                    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_SPEED, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate speed INFO\n");
                        goto err;
                    }
                    #endif
                    speed = pbio_get_uint32_le(lump_dev->rx_msg + 1);
                    if (speed < EV3_UART_SPEED_MIN || speed > EV3_UART_SPEED_MAX) {
                        debug_pr("Speed is out of range\n");
                        goto err;
                    }
                    lump_dev->new_baud_rate = speed;

                    debug_pr("speed: %" PRIu32 "\n", speed);

                    break;
                case LUMP_CMD_WRITE:
                    break;
                case LUMP_CMD_EXT_MODE:
                    // Powered up devices can have modes > LUMP_MAX_MODE. This
                    // command precedes other commands to add the extra 8 to the mode
                    lump_dev->ext_mode = lump_dev->rx_msg[1];
                    break;
                case LUMP_CMD_VERSION:
                    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
                    if (test_and_set_bit(EV3_UART_INFO_BIT_CMD_VERSION, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate version INFO\n");
                        goto err;
                    }
                    // TODO: this might be useful someday
                    debug_pr("fw version: %08" PRIx32 "\n", pbio_get_uint32_le(lump_dev->rx_msg + 1));
                    debug_pr("hw version: %08" PRIx32 "\n", pbio_get_uint32_le(lump_dev->rx_msg + 5));
                    #endif // LUMP_CMD_VERSION
                    break;
                default:
                    debug_pr("Unknown command\n");
                    goto err;
            }
            break;
        case LUMP_MSG_TYPE_INFO:
            switch (cmd2) {
                case LUMP_INFO_NAME: {
                    size_t name_len = 1;
                    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
                    if (lump_dev->rx_msg[2] < 'A' || lump_dev->rx_msg[2] > 'z') {
                        debug_pr("Invalid name INFO\n");
                        goto err;
                    }
                    /*
                    * Name may not have null terminator and we
                    * are done with the checksum at this point
                    * so we are writing 0 over the checksum to
                    * ensure a null terminator for the string
                    * functions.
                    */
                    lump_dev->rx_msg[msg_size - 1] = 0;
                    const char *name = (char *)(lump_dev->rx_msg + 2);
                    name_len = strlen(name);
                    if (name_len > LUMP_MAX_NAME_SIZE) {
                        debug_pr("Name is too long\n");
                        goto err;
                    }
                    lump_dev->info_flags &= ~EV3_UART_INFO_FLAG_ALL_INFO;


                    lump_dev->new_mode = mode;
                    lump_dev->info_flags |= EV3_UART_INFO_FLAG_INFO_NAME;
                    strncpy(lump_dev->parsed_info.mode_info[mode].name, name, name_len);

                    debug_pr("new_mode: %d\n", lump_dev->new_mode);
                    debug_pr("flags: %02X %02X %02X %02X %02X %02X\n",
                        lump_dev->rx_msg[8 + 0], lump_dev->rx_msg[8 + 1], lump_dev->rx_msg[8 + 2],
                        lump_dev->rx_msg[8 + 3], lump_dev->rx_msg[8 + 4], lump_dev->rx_msg[8 + 5]);
                    #endif // PBIO_CONFIG_PORT_LUMP_MODE_INFO

                    // newer LEGO UART devices send additional 6 mode capability flags
                    if (name_len <= LUMP_MAX_SHORT_NAME_SIZE && msg_size > LUMP_MAX_NAME_SIZE) {
                        // Only the first is used in practice.
                        lump_dev->parsed_info.capabilities |= lump_dev->rx_msg[8];
                    }
                    break;
                }
                #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
                case LUMP_INFO_RAW:
                case LUMP_INFO_PCT:
                case LUMP_INFO_SI:
                case LUMP_INFO_UNITS:
                    break;
                case LUMP_INFO_MAPPING:
                    if (lump_dev->new_mode != mode) {
                        debug_pr("Received INFO for incorrect mode\n");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MAPPING, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate mapping INFO\n");
                        goto err;
                    }

                    // Mode supports writing if rx_msg[3] is nonzero.
                    lump_dev->parsed_info.mode_info[mode].writable = lump_dev->rx_msg[3] != 0;

                    debug_pr("mapping: in %02x out %02x\n", lump_dev->rx_msg[2], lump_dev->rx_msg[3]);
                    debug_pr("mapping: in %02x out %02x\n", lump_dev->rx_msg[2], lump_dev->rx_msg[3]);
                    debug_pr("Writable: %d\n", lump_dev->parsed_info.mode_info[mode].writable);

                    break;
                case LUMP_INFO_MODE_COMBOS:
                    if (lump_dev->new_mode != mode) {
                        debug_pr("Received INFO for incorrect mode\n");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_MODE_COMBOS, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate mode combos INFO\n");
                        goto err;
                    }

                    // REVISIT: this is potentially an array of combos
                    debug_pr("mode combos: %04x\n", lump_dev->rx_msg[3] << 8 | lump_dev->rx_msg[2]);

                    break;
                case LUMP_INFO_UNK9:
                    if (lump_dev->new_mode != mode) {
                        debug_pr("Received INFO for incorrect mode\n");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK9, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate UNK9 INFO\n");
                        goto err;
                    }

                    // first 3 parameters look like PID constants, 4th is max tacho_rate
                    debug_pr("motor parameters: %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
                        pbio_get_uint32_le(lump_dev->rx_msg + 2), pbio_get_uint32_le(lump_dev->rx_msg + 6),
                        pbio_get_uint32_le(lump_dev->rx_msg + 10), pbio_get_uint32_le(lump_dev->rx_msg + 14));

                    break;
                case LUMP_INFO_UNK11:
                    if (lump_dev->new_mode != mode) {
                        debug_pr("Received INFO for incorrect mode\n");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_UNK11, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate UNK11 INFO\n");
                        goto err;
                    }
                    break;
                case LUMP_INFO_FORMAT:
                    if (lump_dev->new_mode != mode) {
                        debug_pr("Received INFO for incorrect mode\n");
                        goto err;
                    }
                    if (test_and_set_bit(EV3_UART_INFO_BIT_INFO_FORMAT, &lump_dev->info_flags)) {
                        debug_pr("Received duplicate format INFO\n");
                        goto err;
                    }
                    lump_dev->parsed_info.mode_info[mode].num_values = lump_dev->rx_msg[2];
                    if (!lump_dev->parsed_info.mode_info[mode].num_values) {
                        debug_pr("Invalid number of data sets\n");
                        goto err;
                    }
                    if (msg_size < 7) {
                        debug_pr("Invalid format lump_dev->msg size\n");
                        goto err;
                    }
                    if ((lump_dev->info_flags & EV3_UART_INFO_FLAG_REQUIRED) != EV3_UART_INFO_FLAG_REQUIRED) {
                        debug_pr("Did not receive all required INFO\n");
                        goto err;
                    }
                    lump_dev->parsed_info.mode_info[mode].data_type = lump_dev->rx_msg[3];
                    if (lump_dev->new_mode) {
                        lump_dev->new_mode--;
                    }

                    debug_pr("num_values: %d\n", lump_dev->parsed_info.mode_info[mode].num_values);
                    debug_pr("data_type: %d\n", lump_dev->parsed_info.mode_info[mode].data_type);

                    break;
                #endif // PBIO_CONFIG_PORT_LUMP_MODE_INFO
            }
            break;
        case LUMP_MSG_TYPE_DATA:
            if (lump_dev->status != PBDRV_LEGODEV_LUMP_STATUS_DATA) {
                debug_pr("Received DATA before INFO was complete\n");
                goto err;
            }

            #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
            if (mode >= lump_dev->parsed_info.num_modes) {
                debug_pr("Invalid mode received\n");
                goto err;
            }
            #endif

            // Data is for requested mode.
            if (mode == lump_dev->mode_switch.desired_mode) {
                memcpy(lump_dev->bin_data, lump_dev->rx_msg + 1, msg_size - 2);

                if (lump_dev->mode != mode) {
                    // First time getting data in this mode, so register time.
                    lump_dev->mode_switch.time = pbdrv_clock_get_ms();
                }
            }
            lump_dev->mode = mode;
            pbio_port_lump_handle_known_data(lump_dev);

            lump_dev->data_rec = true;
            if (lump_dev->num_data_err) {
                lump_dev->num_data_err--;
            }
            break;
    }

    return;

err:
    lump_dev->status = PBDRV_LEGODEV_LUMP_STATUS_ERR;
    debug_pr("Data error: %s\n", lump_dev->last_err);
}

static uint8_t ev3_uart_set_msg_hdr(lump_msg_type_t type, lump_msg_size_t size, lump_cmd_t cmd) {
    return (type & LUMP_MSG_TYPE_MASK) | (size & LUMP_MSG_SIZE_MASK) | (cmd & LUMP_MSG_CMD_MASK);
}

static void ev3_uart_prepare_tx_msg(pbio_port_lump_dev_t *lump_dev, lump_msg_type_t msg_type,
    lump_cmd_t cmd, const uint8_t *data, uint8_t len) {
    uint8_t header, checksum, i;
    uint8_t offset = 0;
    lump_msg_size_t size;

    if (msg_type == LUMP_MSG_TYPE_DATA) {
        // Only Powered Up devices support setting data, and they expect to have an
        // extra command sent to give the part of the mode > 7
        lump_dev->tx_msg[0] = ev3_uart_set_msg_hdr(LUMP_MSG_TYPE_CMD, LUMP_MSG_SIZE_1, LUMP_CMD_EXT_MODE);
        lump_dev->tx_msg[1] = lump_dev->mode > LUMP_MAX_MODE ? 8 : 0;
        lump_dev->tx_msg[2] = 0xff ^ lump_dev->tx_msg[0] ^ lump_dev->tx_msg[1];
        offset = 3;
    }

    checksum = 0xff;
    for (i = 0; i < len; i++) {
        lump_dev->tx_msg[offset + i + 1] = data[i];
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
        lump_dev->tx_msg[offset + i + 1] = 0;
    }

    header = ev3_uart_set_msg_hdr(msg_type, size, cmd);
    checksum ^= header;

    lump_dev->tx_msg[offset] = header;
    lump_dev->tx_msg[offset + i + 1] = checksum;
    lump_dev->tx_msg_size = offset + i + 2;
}

/**
 * The synchronization thread for the LEGO UART device.
 *
 * This thread receives and parses incoming messages sent by LEGO UART devices
 * when they are plugged in. It populates the device state accordingly.
 *
 * @param [in]  pt             The protothread.
 * @param [in]  lump_dev       The LEGO UART device instance.
 * @param [in]  uart_dev       The UART device instance.
 * @param [in]  etimer         The timer for the protothread.
 * @param [out] parsed_info    The device information.
 */
PT_THREAD(pbio_port_lump_sync_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev, struct etimer *etimer, pbio_port_device_info_t *device_info)) {
    PT_BEGIN(pt);

    // Reset state for new device
    lump_dev->ext_mode = 0;
    lump_dev->status = PBDRV_LEGODEV_LUMP_STATUS_SYNCING;

    // Send SPEED command at 115200 baud
    debug_pr("set baud: %d\n", EV3_UART_SPEED_LPF2);
    pbdrv_uart_set_baud_rate(uart_dev, EV3_UART_SPEED_LPF2);
    uint8_t speed_payload[4];
    pbio_set_uint32_le(speed_payload, EV3_UART_SPEED_LPF2);
    ev3_uart_prepare_tx_msg(lump_dev, LUMP_MSG_TYPE_CMD, LUMP_CMD_SPEED, speed_payload, sizeof(speed_payload));

    pbdrv_uart_flush(uart_dev);

    PT_SPAWN(pt, &lump_dev->write_pt, pbdrv_uart_write(&lump_dev->write_pt, uart_dev, lump_dev->tx_msg, lump_dev->tx_msg_size, EV3_UART_IO_TIMEOUT, &lump_dev->err));
    if (lump_dev->err != PBIO_SUCCESS) {
        debug_pr("Sp tx fail %d\n", lump_dev->err);
        PT_EXIT(pt);
    }

    pbdrv_uart_flush(uart_dev);

    // read one byte to check for ACK
    PT_SPAWN(pt, &lump_dev->read_pt, pbdrv_uart_read(&lump_dev->read_pt, uart_dev, lump_dev->rx_msg, 1, 10, &lump_dev->err));

    if ((lump_dev->err == PBIO_SUCCESS && lump_dev->rx_msg[0] != LUMP_SYS_ACK) || lump_dev->err == PBIO_ERROR_TIMEDOUT) {
        // if we did not get ACK within 100ms, then switch to slow baud rate for sync
        pbdrv_uart_set_baud_rate(uart_dev, EV3_UART_SPEED_MIN);
        debug_pr("set baud: %d\n", EV3_UART_SPEED_MIN);
    } else if (lump_dev->err != PBIO_SUCCESS) {
        debug_pr("UART Rx error during baud\n");
        PT_EXIT(pt);
    }
sync:

    // To get in sync with the data stream from the sensor, we look for a valid TYPE command.
    for (;;) {
        // If there are multiple bytes waiting to be read, this drains them one
        // by one without requiring additional polls. This means we won't need
        // exact timing to get in sync.
        PT_SPAWN(pt, &lump_dev->read_pt, pbdrv_uart_read(&lump_dev->read_pt, uart_dev, lump_dev->rx_msg, 1, EV3_UART_IO_TIMEOUT, &lump_dev->err));
        if (lump_dev->err == PBIO_ERROR_TIMEDOUT) {
            continue;
        }
        if (lump_dev->err != PBIO_SUCCESS) {
            debug_pr("UART Rx error during sync\n");
            PT_EXIT(pt);
        }

        if (lump_dev->rx_msg[0] == (LUMP_MSG_TYPE_CMD | LUMP_CMD_TYPE)) {
            break;
        }
    }

    // then read the rest of the message
    PT_SPAWN(pt, &lump_dev->read_pt, pbdrv_uart_read(&lump_dev->read_pt, uart_dev, lump_dev->rx_msg + 1, 2, EV3_UART_IO_TIMEOUT, &lump_dev->err));
    if (lump_dev->err != PBIO_SUCCESS) {
        debug_pr("UART Rx error while reading type\n");
        PT_EXIT(pt);
    }

    bool bad_id = lump_dev->rx_msg[1] < EV3_UART_TYPE_MIN || lump_dev->rx_msg[1] > EV3_UART_TYPE_MAX;
    uint8_t checksum = 0xff ^ lump_dev->rx_msg[0] ^ lump_dev->rx_msg[1];
    bool bad_id_checksum = lump_dev->rx_msg[2] != checksum;

    if (bad_id || bad_id_checksum) {
        debug_pr("Bad device type id or checksum\n");
        if (lump_dev->err_count > 10) {
            lump_dev->err_count = 0;
            lump_dev->err = PBIO_ERROR_IO;
            PT_EXIT(pt);
        }
        lump_dev->err_count++;
        goto sync;
    }

    // if all was good, we are ready to start receiving the mode info
    lump_dev->parsed_info.type_id = lump_dev->rx_msg[1];
    lump_dev->data_rec = false;
    lump_dev->num_data_err = 0;
    lump_dev->status = PBDRV_LEGODEV_LUMP_STATUS_INFO;
    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
    lump_dev->info_flags = EV3_UART_INFO_FLAG_CMD_TYPE;
    lump_dev->parsed_info.num_modes = 1;
    #endif
    debug_pr("type id: %d\n", lump_dev->parsed_info.type_id);

    while (lump_dev->status == PBDRV_LEGODEV_LUMP_STATUS_INFO) {
        // read the message header
        PT_SPAWN(pt, &lump_dev->read_pt, pbdrv_uart_read(&lump_dev->read_pt, uart_dev, lump_dev->rx_msg, 1, EV3_UART_IO_TIMEOUT, &lump_dev->err));
        if (lump_dev->err != PBIO_SUCCESS) {
            debug_pr("UART Rx end error during info header\n");
            PT_EXIT(pt);
        }

        lump_dev->rx_msg_size = ev3_uart_get_msg_size(lump_dev->rx_msg[0]);
        if (lump_dev->rx_msg_size > EV3_UART_MAX_MESSAGE_SIZE) {
            debug_pr("Bad message size during info\n");
            lump_dev->err = PBIO_ERROR_IO;
            PT_EXIT(pt);
        }

        // read the rest of the message
        if (lump_dev->rx_msg_size > 1) {
            PT_SPAWN(pt, &lump_dev->read_pt, pbdrv_uart_read(&lump_dev->read_pt, uart_dev, lump_dev->rx_msg + 1, lump_dev->rx_msg_size - 1, EV3_UART_IO_TIMEOUT, &lump_dev->err));
            if (lump_dev->err != PBIO_SUCCESS) {
                debug_pr("UART Rx end error during info\n");
                PT_EXIT(pt);
            }
        }

        // at this point, we have a full lump_dev->msg that can be parsed
        pbio_port_lump_lump_parse_msg(lump_dev);
    }

    // at this point we should have read all of the mode info
    if (lump_dev->status != PBDRV_LEGODEV_LUMP_STATUS_ACK) {
        lump_dev->err = PBIO_ERROR_IO;
        PT_EXIT(pt);
    }

    // reply with ACK
    lump_dev->tx_msg[0] = LUMP_SYS_ACK;
    lump_dev->tx_msg_size = 1;
    PT_SPAWN(pt, &lump_dev->write_pt, pbdrv_uart_write(&lump_dev->write_pt, uart_dev, lump_dev->tx_msg, lump_dev->tx_msg_size, EV3_UART_IO_TIMEOUT, &lump_dev->err));
    if (lump_dev->err != PBIO_SUCCESS) {
        debug_pr("UART Tx error during ack.\n");
        PT_EXIT(pt);
    }

    // schedule baud rate change
    etimer_set(&lump_dev->timer, 10);
    PT_WAIT_UNTIL(pt, etimer_expired(&lump_dev->timer));

    // change the baud rate
    pbdrv_uart_set_baud_rate(uart_dev, lump_dev->new_baud_rate);
    debug_pr("set baud: %" PRIu32 "\n", lump_dev->new_baud_rate);

    // Map device capabilities and power requirements to practically useful
    // device categories for use in Pybricks.
    device_info->type_id = lump_dev->parsed_info.type_id;
    if (device_info->type_id == LEGO_DEVICE_TYPE_ID_INTERACTIVE_MOTOR) {
        // Only this device is a relative motor, and it does not send flags.
        device_info->kind = PBIO_PORT_DEVICE_KIND_LUMP_MOTOR_RELATIVE;
    }
    if (lump_dev->parsed_info.capabilities & LUMP_MODE_FLAGS0_MOTOR_ABS_POS) {
        device_info->kind = PBIO_PORT_DEVICE_KIND_LUMP_MOTOR_ABSOLUTE;
    }
    if (lump_dev->parsed_info.capabilities & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN1) {
        device_info->power_requirements = PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS;
    }
    if (lump_dev->parsed_info.capabilities & LUMP_MODE_FLAGS0_NEEDS_SUPPLY_PIN2) {
        device_info->power_requirements = PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P2_POS;
    }

    // Request switch to default mode for this device.
    uint8_t default_mode = 0;
    if (device_info->kind == PBIO_PORT_DEVICE_KIND_LUMP_MOTOR_ABSOLUTE) {
        default_mode = LEGO_DEVICE_MODE_PUP_ABS_MOTOR__CALIB;
    } else if (device_info->kind == PBIO_PORT_DEVICE_KIND_LUMP_MOTOR_RELATIVE) {
        default_mode = LEGO_DEVICE_MODE_PUP_REL_MOTOR__POS;
    } else if (device_info->type_id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR) {
        default_mode = LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I;
    }
    pbio_port_lump_request_mode(lump_dev, default_mode);

    // Reset other timers
    etimer_reset_with_new_interval(&lump_dev->timer, EV3_UART_DATA_KEEP_ALIVE_TIMEOUT);
    lump_dev->data_set->time = pbdrv_clock_get_ms();
    lump_dev->data_set->size = 0;

    lump_dev->status = PBDRV_LEGODEV_LUMP_STATUS_DATA;

    PT_END(pt);
}

/**
 * The send thread for the LEGO UART device.
 *
 * Responsible for sending keep alive messages and scheduled mode changes.
 *
 * @param [in]  pt             The protothread.
 * @param [in]  lump_dev       The LEGO UART device instance.
 * @param [in]  uart_dev       The UART device instance.
 * @param [in]  etimer         The timer for the protothread.
 */
PT_THREAD(pbio_port_lump_data_send_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev, struct etimer *etimer)) {
    PT_BEGIN(pt);


    while (lump_dev->status == PBDRV_LEGODEV_LUMP_STATUS_DATA) {

        PT_WAIT_UNTIL(pt, etimer_expired(&lump_dev->timer) || lump_dev->mode_switch.requested || lump_dev->data_set->size > 0);

        // Handle keep alive timeout
        if (etimer_expired(&lump_dev->timer)) {
            // make sure we are receiving data
            if (!lump_dev->data_rec) {
                lump_dev->num_data_err++;
                debug_pr("No data since last keepalive\n");
                if (lump_dev->num_data_err > 6) {
                    lump_dev->err = PBIO_ERROR_IO;
                    PT_EXIT(pt);
                }
            }
            lump_dev->data_rec = false;
            lump_dev->tx_msg[0] = LUMP_SYS_NACK;
            lump_dev->tx_msg_size = 1;
            PT_SPAWN(pt, &lump_dev->write_pt, pbdrv_uart_write(&lump_dev->write_pt, uart_dev, lump_dev->tx_msg, lump_dev->tx_msg_size, EV3_UART_IO_TIMEOUT, &lump_dev->err));
            if (lump_dev->err != PBIO_SUCCESS) {
                debug_pr("Error during keepalive.\n");
                PT_EXIT(pt);
            }
            etimer_reset_with_new_interval(&lump_dev->timer, EV3_UART_DATA_KEEP_ALIVE_TIMEOUT);

            // Retry mode switch if it hasn't been handled or failed.
            if (lump_dev->mode != lump_dev->mode_switch.desired_mode && pbdrv_clock_get_ms() - lump_dev->mode_switch.time > EV3_UART_IO_TIMEOUT) {
                lump_dev->mode_switch.requested = true;
            }
        }

        // Handle requested mode change
        if (lump_dev->mode_switch.requested) {
            lump_dev->mode_switch.requested = false;
            ev3_uart_prepare_tx_msg(lump_dev, LUMP_MSG_TYPE_CMD, LUMP_CMD_SELECT, &lump_dev->mode_switch.desired_mode, 1);
            PT_SPAWN(pt, &lump_dev->write_pt, pbdrv_uart_write(&lump_dev->write_pt, uart_dev, lump_dev->tx_msg, lump_dev->tx_msg_size, EV3_UART_IO_TIMEOUT, &lump_dev->err));
            if (lump_dev->err != PBIO_SUCCESS) {
                debug_pr("Setting requested mode failed.\n");
                PT_EXIT(pt);
            }
        }

        // Handle requested data set
        if (lump_dev->data_set->size > 0) {
            // Only set data if we are in the correct mode already.
            if (lump_dev->mode == lump_dev->data_set->desired_mode) {
                ev3_uart_prepare_tx_msg(lump_dev, LUMP_MSG_TYPE_DATA, lump_dev->data_set->desired_mode, lump_dev->data_set->bin_data, lump_dev->data_set->size);
                lump_dev->data_set->size = 0;
                lump_dev->data_set->time = pbdrv_clock_get_ms();
                PT_SPAWN(pt, &lump_dev->write_pt, pbdrv_uart_write(&lump_dev->write_pt, uart_dev, lump_dev->tx_msg, lump_dev->tx_msg_size, EV3_UART_IO_TIMEOUT, &lump_dev->err));
                if (lump_dev->err != PBIO_SUCCESS) {
                    debug_pr("Setting requested data failed.\n");
                    PT_EXIT(pt);
                }
                lump_dev->data_set->time = pbdrv_clock_get_ms();
            } else if (pbdrv_clock_get_ms() - lump_dev->data_set->time < 500) {
                // Not in the right mode yet, try again later for a reasonable amount of time.
                pbio_port_process_poll(lump_dev->port);
                PT_YIELD(pt);
            } else {
                // Give up setting data.
                lump_dev->data_set->size = 0;
            }
        }
    }

    PT_END(pt);
}

/**
 * The receive thread for the LEGO UART device.
 *
 * Responsible for receiving data messages and updating mode switch completion state.
 *
 * @param [in]  pt             The protothread.
 * @param [in]  lump_dev       The LEGO UART device instance.
 * @param [in]  uart_dev       The UART device instance.
 */
PT_THREAD(pbio_port_lump_data_recv_thread(struct pt *pt, pbio_port_lump_dev_t *lump_dev, pbdrv_uart_dev_t *uart_dev)) {

    // REVISIT: This is not the greatest. We can easily get a buffer overrun and
    // loose data. For now, the retry after bad message size helps get back into
    // sync with the data stream.

    PT_BEGIN(pt);

    while (true) {
        PT_SPAWN(pt, &lump_dev->read_pt, pbdrv_uart_read(&lump_dev->read_pt, uart_dev, lump_dev->rx_msg, 1, EV3_UART_IO_TIMEOUT, &lump_dev->err));
        if (lump_dev->err != PBIO_SUCCESS) {
            debug_pr("UART Rx data header end error\n");
            break;
        }

        lump_dev->rx_msg_size = ev3_uart_get_msg_size(lump_dev->rx_msg[0]);
        if (lump_dev->rx_msg_size < 3 || lump_dev->rx_msg_size > EV3_UART_MAX_MESSAGE_SIZE) {
            debug_pr("Bad data message size\n");
            continue;
        }

        uint8_t msg_type = lump_dev->rx_msg[0] & LUMP_MSG_TYPE_MASK;
        uint8_t cmd = lump_dev->rx_msg[0] & LUMP_MSG_CMD_MASK;
        if (msg_type != LUMP_MSG_TYPE_DATA && (msg_type != LUMP_MSG_TYPE_CMD ||
                                               (cmd != LUMP_CMD_WRITE && cmd != LUMP_CMD_EXT_MODE))) {
            debug_pr("Bad msg type\n");
            continue;
        }

        PT_SPAWN(pt, &lump_dev->read_pt, pbdrv_uart_read(&lump_dev->read_pt, uart_dev, lump_dev->rx_msg + 1, lump_dev->rx_msg_size - 1, EV3_UART_IO_TIMEOUT, &lump_dev->err));
        if (lump_dev->err != PBIO_SUCCESS) {
            debug_pr("UART Rx data end error\n");
            break;
        }

        // at this point, we have a full lump_dev->msg that can be parsed
        pbio_port_lump_lump_parse_msg(lump_dev);
    }

    PT_END(pt);
}

/**
 * Gets the size of a data type.
 *
 * @param [in]  type        The data type
 * @return                  The size of the type or 0 if the type was not valid
 */
size_t pbio_port_lump_data_size(lump_data_type_t type) {
    switch (type) {
        case LUMP_DATA_TYPE_DATA8:
            return 1;
        case LUMP_DATA_TYPE_DATA16:
            return 2;
        case LUMP_DATA_TYPE_DATA32:
        case LUMP_DATA_TYPE_DATAF:
            return 4;
    }
    return 0;
}

/**
 * Checks if LEGO UART device has data available for reading or is ready to write.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @return                  ::PBIO_SUCCESS if ready.
 *                          ::PBIO_ERROR_AGAIN if not ready yet.
 *                          ::PBIO_ERROR_NO_DEV if no device is attached.
 */
pbio_error_t pbio_port_lump_is_ready(pbio_port_lump_dev_t *lump_dev) {

    if (!lump_dev || lump_dev->status == PBDRV_LEGODEV_LUMP_STATUS_ERR) {
        return PBIO_ERROR_NO_DEV;
    }

    if (lump_dev->status != PBDRV_LEGODEV_LUMP_STATUS_DATA) {
        return PBIO_ERROR_AGAIN;
    }

    uint32_t time = pbdrv_clock_get_ms();

    // Not ready if waiting for mode change
    if (lump_dev->mode != lump_dev->mode_switch.desired_mode) {
        return PBIO_ERROR_AGAIN;
    }

    // Not ready if waiting for stale data to be discarded.
    if (time - lump_dev->mode_switch.time <= lego_device_stale_data_delay(lump_dev->parsed_info.type_id, lump_dev->mode)) {
        return PBIO_ERROR_AGAIN;
    }

    // Not ready if just recently set new data.
    if (lump_dev->data_set->size > 0 || time - lump_dev->data_set->time <= lego_device_data_set_delay(lump_dev->parsed_info.type_id, lump_dev->mode)) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Starts setting the mode of a LEGO UART device.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [in]  id          The ID of the device to request data from.
 * @param [in]  mode        The mode to set.
 * @return                  ::PBIO_SUCCESS on success or if mode already set.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 *                          ::PBIO_ERROR_INVALID_ARG if the mode is not valid.
 *                          ::PBIO_ERROR_AGAIN if the device is not ready for this operation.
 */
pbio_error_t pbio_port_lump_set_mode(pbio_port_lump_dev_t *lump_dev, uint8_t mode) {

    if (!lump_dev) {
        return PBIO_ERROR_NO_DEV;
    }

    // Mode already set or being set, so return success.
    if (lump_dev->mode_switch.desired_mode == mode || lump_dev->mode == mode) {
        return PBIO_SUCCESS;
    }

    // We can only initiate a mode switch if currently idle (receiving data).
    pbio_error_t err = pbio_port_lump_is_ready(lump_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
    // Can only set available modes.
    if (mode >= lump_dev->parsed_info.num_modes) {
        return PBIO_ERROR_INVALID_ARG;
    }
    #endif

    // Request mode switch.
    pbio_port_lump_request_mode(lump_dev, mode);

    return PBIO_SUCCESS;
}

/**
 * Atomic operation for asserting the mode/id and getting the data of a LEGO UART device.
 *
 * The returned data buffer is 4-byte aligned. Data is in little-endian format.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [out] data        Pointer to hold array of data values.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 *                          ::PBIO_ERROR_AGAIN if the device is not ready for this operation.
 */
pbio_error_t pbio_port_lump_get_data(pbio_port_lump_dev_t *lump_dev, uint8_t mode, void **data) {

    if (!lump_dev) {
        return PBIO_ERROR_NO_DEV;
    }

    // Can only request data for mode that is set.
    if (mode != lump_dev->mode) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Can only read if ready.
    *data = lump_dev->bin_data;
    return pbio_port_lump_is_ready(lump_dev);
}

/**
 * Set data for the current mode.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [out] data        Data to be set.
 * @param [in]  size        Size of data to be set.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached.
 */
pbio_error_t pbio_port_lump_set_mode_with_data(pbio_port_lump_dev_t *lump_dev, uint8_t mode, const void *data, uint8_t size) {

    if (!lump_dev) {
        return PBIO_ERROR_NO_DEV;
    }

    #if PBIO_CONFIG_PORT_LUMP_MODE_INFO
    const pbio_port_lump_mode_info_t *mode_info = &lump_dev->parsed_info.mode_info[mode];
    // Not all modes support setting data and data must be of expected size.
    if (!mode_info->writable || size != mode_info->num_values * pbio_port_lump_data_size(mode_info->data_type)) {
        return PBIO_ERROR_INVALID_OP;
    }
    #endif

    // Start setting mode.
    pbio_error_t err = pbio_port_lump_set_mode(lump_dev, mode);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Request data set.
    pbio_port_lump_request_data_set(lump_dev, mode, data, size);
    return PBIO_SUCCESS;
}

/**
 * Get the LEGO UART device information.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [out] info        The device information.
 * @return                  Error code.
 */
pbio_error_t pbio_port_lump_get_info(pbio_port_lump_dev_t *lump_dev, pbio_port_lump_device_info_t **info, uint8_t *current_mode) {

    if (!lump_dev) {
        return PBIO_ERROR_NO_DEV;
    }
    // Info is set even in case of error. Caller can decide what values apply
    // based on the error code.
    *info = &lump_dev->parsed_info;
    *current_mode = lump_dev->mode;
    return pbio_port_lump_is_ready(lump_dev);
}

#endif // PBIO_CONFIG_PORT && PBIO_CONFIG_PORT_LUMP_NUM > 0

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Bluetooth driver for the LEGO MINDSTORMS NXT.
//
// The NXT has a CSR BlueCore 4 (BC4) running LEGO's own firmware. It is not an
// HCI controller: it runs the whole Bluetooth stack itself and exposes only a
// small command protocol on USART1. Once a Serial Port Profile (RFCOMM)
// connection is up, a GPIO switches that same UART to a raw byte stream, so
// commands and data are mutually exclusive.
//
// Only the server role is implemented for now: the hub makes itself
// discoverable and waits for a host to pair with it and open the serial port.
// Inquiry scanning and outbound pairing, which the BC4 also supports, are
// stubbed out until there is a user interface to drive them.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_NXT

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <at91sam7s256.h>

#include <lwrb/lwrb.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/display.h>

#include <pbio/busy_count.h>
#include <pbio/error.h>
#include <pbio/image.h>
#include <pbio/os.h>
#include <pbio/serial.h>
#include <pbio/util.h>

#include <pbsys/host.h>

#include "bluetooth_nxt.h"

#include "../rproc/rproc.h"

#include <pbdrv/adc.h>

#include "nxos/drivers/aic.h"
#include "nxos/interrupts.h"
#include "nxos/nxt.h"


#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

// BC4 control lines.
#define BT_RST_PIN      AT91C_PIO_PA11  // Low holds the BC4 in reset.
#define BT_ARM_CMD_PIN  AT91C_PIO_PA27  // Low: command mode, high: stream mode.

// CSBT. leJOS drives this high as an output before releasing reset, so it is
// presumably sampled by the BC4 to select how it boots.
#define BT_CS_PIN       AT91C_PIO_PA31

// Asynchronous mode with hardware handshaking, so the synchronous clock on
// PA23 is left alone. That pin is sensor port 1 pin 5.
#define BT_UART_PIOA_PINS \
    (AT91C_PA21_RXD1 | AT91C_PA22_TXD1 | AT91C_PA24_RTS1 | AT91C_PA25_CTS1)

// 460.8 kBaud. The divisor programs 461.5 kBaud, which is within tolerance.
#define BT_UART_CLOCK_DIVISOR (NXT_CLOCK_FREQ / 8 / 460800)

/** Time to wait for the BC4 to reply to a command. */
#define BT_REPLY_TIMEOUT_MS (3000)

/** Time to wait for the BC4 to boot and announce itself. */
#define BT_RESET_TIMEOUT_MS (5000)

/**
 * PIN code sent when a host asks for one. The BC4 predates Secure Simple
 * Pairing, so the host always prompts for this.
 */
#define BT_PIN_CODE "1234"

/** Size of a BC4 address: six address bytes plus a zero terminator. */
#define BT_ADDR_SIZE (7)

/** Maximum length of a friendly name or PIN code accepted by the BC4. */
#define BT_NAME_SIZE (16)
#define BT_PIN_SIZE (16)

/** Longest message in either direction is ADD_DEVICE / LIST_ITEM at 30 bytes. */
#define BT_MSG_SIZE_MAX (30)

typedef enum {
    // ARM7 to BC4.
    BT_MSG_OPEN_PORT = 0x03,
    BT_MSG_CLOSE_CONNECTION = 0x08,
    BT_MSG_ACCEPT_CONNECTION = 0x09,
    BT_MSG_PIN_CODE = 0x0A,
    BT_MSG_OPEN_STREAM = 0x0B,
    BT_MSG_SET_DISCOVERABLE = 0x1C,
    BT_MSG_SET_FRIENDLY_NAME = 0x21,
    BT_MSG_GET_LOCAL_ADDR = 0x27,
    // BC4 to ARM7.
    BT_MSG_HEARTBEAT = 0x0D,
    BT_MSG_CONNECT_RESULT = 0x13,
    BT_MSG_RESET_INDICATION = 0x14,
    BT_MSG_REQUEST_PIN_CODE = 0x15,
    BT_MSG_REQUEST_CONNECTION = 0x16,
    BT_MSG_CLOSE_CONNECTION_RESULT = 0x1A,
    BT_MSG_PORT_OPEN_RESULT = 0x1B,
    BT_MSG_PIN_CODE_ACK = 0x1F,
    BT_MSG_SET_DISCOVERABLE_ACK = 0x20,
    BT_MSG_SET_FRIENDLY_NAME_ACK = 0x22,
    BT_MSG_GET_LOCAL_ADDR_RESULT = 0x28,
    /** Not a BC4 message: used to mean that no reply is expected. */
    BT_MSG_NONE = 0xFF,
} bt_msg_t;

/** Address of this hub, once the BC4 has reported it. */
static uint8_t bt_local_addr[6];
static bool bt_local_addr_valid;

/** Received stream bytes, written by the interrupt and drained by pbio serial. */
static lwrb_t bt_rx_ring;
static uint8_t bt_rx_ring_buf[256];

/** Command message being assembled for transmission. */
static uint8_t bt_tx_buf[BT_MSG_SIZE_MAX + 1];

static volatile struct {
    /** Message being assembled in command mode, excluding the length byte. */
    uint8_t rx_msg[BT_MSG_SIZE_MAX];
    /** Number of bytes received so far, and the total expected, or 0 if unknown. */
    uint8_t rx_msg_len;
    uint8_t rx_msg_size;
    /** Message type the process thread is waiting for, if any. */
    uint8_t awaiting;
    bool reply_received;
    /** Arguments of the awaited reply, excluding type and checksum. */
    uint8_t reply_args[BT_MSG_SIZE_MAX];
    /** Set when the BC4 announces that it has (re)started. */
    bool reset_indication;
    /** Address of the device asking for a PIN code or connection, if any. */
    uint8_t request_addr[BT_ADDR_SIZE];
    bool pin_requested;
    bool connection_requested;
    /** Connection handle reported by the BC4, or -1 if there is no connection. */
    int8_t handle;
    /** Whether the UART carries a raw byte stream instead of commands. */
    bool streaming;
    /** Set on soft-poweroff, to drop the link before the system goes down. */
    bool closing;
    /** Set when the BC4 sends a break condition. */
    bool break_received;
    /** Number of messages dropped because of a bad checksum. */
    uint16_t checksum_errors;
} bt;

//
// USART1
//

static bool bt_uart_is_writing(void) {
    return (*AT91C_US1_TCR + *AT91C_US1_TNCR) > 0;
}

/**
 * Hands @p data to the peripheral DMA controller. It must stay valid until the
 * transfer completes, and the transmitter must be idle.
 */
static void bt_uart_write(const uint8_t *data, uint32_t size) {
    *AT91C_US1_TNPR = (uint32_t)data;
    *AT91C_US1_TNCR = size;
    *AT91C_US1_IER = AT91C_US_TXBUFE;
}

/**
 * Validates the trailing 16-bit checksum. The BC4 counts the length byte in
 * the messages it sends, but not in the ones it accepts.
 */
static bool bt_checksum_is_valid(volatile uint8_t *msg, uint8_t size) {
    uint32_t sum = size;
    for (uint8_t i = 0; i < size - 2; i++) {
        sum += msg[i];
    }
    sum = -sum;
    return msg[size - 2] == ((sum >> 8) & 0xFF) && msg[size - 1] == (sum & 0xFF);
}

static void bt_handle_message(volatile uint8_t *msg, uint8_t size) {

    if (size < 3 || !bt_checksum_is_valid(msg, size)) {
        bt.checksum_errors++;
        return;
    }

    volatile uint8_t *args = &msg[1];
    uint8_t args_size = size - 3;

    switch (msg[0]) {
        case BT_MSG_HEARTBEAT:
            return;
        case BT_MSG_RESET_INDICATION:
            bt.reset_indication = true;
            break;
        case BT_MSG_REQUEST_PIN_CODE:
        case BT_MSG_REQUEST_CONNECTION:
            for (uint8_t i = 0; i < BT_ADDR_SIZE && i < args_size; i++) {
                bt.request_addr[i] = args[i];
            }
            if (msg[0] == BT_MSG_REQUEST_PIN_CODE) {
                bt.pin_requested = true;
            } else {
                bt.connection_requested = true;
            }
            break;
        case BT_MSG_CONNECT_RESULT:
            // First argument is the status, second is the handle.
            if (args_size >= 2 && args[0]) {
                bt.handle = args[1];
            }
            break;
        case BT_MSG_CLOSE_CONNECTION_RESULT:
            bt.handle = -1;
            break;
        default:
            break;
    }

    if (msg[0] == bt.awaiting && !bt.reply_received) {
        for (uint8_t i = 0; i < args_size; i++) {
            bt.reply_args[i] = args[i];
        }
        bt.reply_received = true;
    }
}

static void bt_uart_isr(void) {

    uint32_t status = *AT91C_US1_CSR;

    if (status & AT91C_US_RXBRK) {
        // A line held low keeps re-asserting this, so only the first one is
        // of interest. Stream mode enables it again.
        *AT91C_US1_CR = AT91C_US_RSTSTA;
        *AT91C_US1_IDR = AT91C_US_RXBRK;
        bt.break_received = true;
    }

    if (status & AT91C_US_TXBUFE) {
        *AT91C_US1_IDR = AT91C_US_TXBUFE;
    }

    while (*AT91C_US1_CSR & AT91C_US_RXRDY) {
        uint8_t byte = *AT91C_US1_RHR;

        if (bt.streaming) {
            // If the ring is full, excess bytes are dropped and the COBS
            // framing in the pbio serial process resyncs on the next frame.
            lwrb_write(&bt_rx_ring, &byte, 1);
            continue;
        }

        // Command messages are self delimiting: the first byte is the number
        // of bytes that follow, including the two checksum bytes.
        if (bt.rx_msg_size == 0) {
            bt.rx_msg_size = byte <= BT_MSG_SIZE_MAX ? byte : 0;
            continue;
        }

        bt.rx_msg[bt.rx_msg_len++] = byte;
        if (bt.rx_msg_len >= bt.rx_msg_size) {
            bt_handle_message(bt.rx_msg, bt.rx_msg_size);
            bt.rx_msg_len = 0;
            bt.rx_msg_size = 0;
        }
    }

    pbio_os_request_poll();
}

static void bt_uart_init(void) {

    uint32_t state = nx_interrupts_disable();

    *AT91C_PMC_PCER = (1 << AT91C_ID_US1);

    *AT91C_PIOA_PDR = BT_UART_PIOA_PINS;
    *AT91C_PIOA_ASR = BT_UART_PIOA_PINS;

    *AT91C_US1_CR = AT91C_US_TXDIS | AT91C_US_RXDIS;
    *AT91C_US1_IDR = ~0;
    *AT91C_US1_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA | AT91C_US_RSTNACK;

    // Receiving is done one byte at a time from the holding register, but the
    // receive counter must be non-zero for the holding register to fill up,
    // even with the receiving side of the DMA controller disabled.
    static uint8_t rx_dma_dummy[1];
    *AT91C_US1_PTCR = AT91C_PDC_RXTDIS;
    *AT91C_US1_RPR = (uint32_t)rx_dma_dummy;
    *AT91C_US1_RCR = sizeof(rx_dma_dummy);
    *AT91C_US1_RNPR = 0;
    *AT91C_US1_RNCR = 0;
    *AT91C_US1_TPR = 0;
    *AT91C_US1_TCR = 0;
    *AT91C_US1_TNPR = 0;
    *AT91C_US1_TNCR = 0;

    // Hardware handshaking, master clock, 8N1, no receive timeout.
    *AT91C_US1_MR = AT91C_US_USMODE_HWHSH | AT91C_US_CLKS_CLOCK |
        AT91C_US_CHRL_8_BITS | AT91C_US_NBSTOP_1_BIT | AT91C_US_PAR_NONE |
        AT91C_US_CHMODE_NORMAL | AT91C_US_OVER;
    *AT91C_US1_BRGR = BT_UART_CLOCK_DIVISOR;
    *AT91C_US1_RTOR = 0;
    *AT91C_US1_CR = AT91C_US_STTTO;

    nx_aic_install_isr(AT91C_ID_US1, AIC_PRIO_DRIVER, AIC_TRIG_LEVEL, bt_uart_isr);
    *AT91C_US1_IER = AT91C_US_RXRDY;

    *AT91C_US1_CR = AT91C_US_TXEN | AT91C_US_RXEN;
    *AT91C_US1_PTCR = AT91C_PDC_TXTEN;

    nx_interrupts_enable(state);
}

//
// BC4 command protocol
//

/**
 * Composes a command message and hands it to the transmitter, which must be
 * idle.
 */
static void bt_send(uint8_t type, const uint8_t *args, uint8_t args_size) {

    // The length byte counts everything after it, including the checksum.
    uint8_t size = 1 + args_size + 2;
    bt_tx_buf[0] = size;
    bt_tx_buf[1] = type;
    memcpy(&bt_tx_buf[2], args, args_size);

    uint32_t sum = 0;
    for (uint8_t i = 1; i <= size - 2; i++) {
        sum += bt_tx_buf[i];
    }
    sum = -sum;
    bt_tx_buf[size - 1] = (sum >> 8) & 0xFF;
    bt_tx_buf[size] = sum & 0xFF;

    bt_uart_write(bt_tx_buf, size + 1);
}

/**
 * Sends a command and awaits the message the BC4 answers it with, or returns
 * as soon as it has been sent if @p reply is ::BT_MSG_NONE.
 *
 * Only one command can be outstanding at a time, which is why this may not be
 * awaited from more than one thread.
 */
static pbio_error_t bt_command(pbio_os_state_t *state, uint8_t type, const uint8_t *args, uint8_t args_size, uint8_t reply) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // The message buffer is handed to the DMA controller, so the previous one
    // has to be out before it can be composed anew. Hardware handshaking means
    // this never completes if the BC4 is not there to accept it.
    pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, !bt_uart_is_writing() || pbio_os_timer_is_expired(&timer));
    if (bt_uart_is_writing()) {
        DEBUG_PRINT("tx stall\n");
        return PBIO_ERROR_TIMEDOUT;
    }

    bt.reply_received = false;
    bt.awaiting = reply;
    bt_send(type, args, args_size);

    if (reply == BT_MSG_NONE) {
        return PBIO_SUCCESS;
    }

    pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, bt.reply_received || pbio_os_timer_is_expired(&timer));

    bt.awaiting = BT_MSG_NONE;
    if (!bt.reply_received) {
        DEBUG_PRINT("no ack %x\n", type);
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Whether the BC4 still has the UART switched to stream mode.
 *
 * The BC4 drives this line low when it needs command mode back, such as when
 * the remote device disconnects. It is wired to the ARM's own ADC rather than
 * to a digital input.
 */
static bool bt_bc4_is_streaming(void) {
    uint16_t adc;
    return pbdrv_adc_get_ch(6, &adc) == PBIO_SUCCESS ? adc > 512 : false;
}

/**
 * Switches the UART between the command protocol and the raw byte stream of an
 * open serial port connection.
 */
static void bt_set_streaming(bool streaming) {

    uint32_t state = nx_interrupts_disable();

    if (streaming) {
        *AT91C_PIOA_SODR = BT_ARM_CMD_PIN;
    } else {
        *AT91C_PIOA_CODR = BT_ARM_CMD_PIN;
    }

    bt.rx_msg_len = 0;
    bt.rx_msg_size = 0;
    bt.break_received = false;
    bt.streaming = streaming;
    lwrb_reset(&bt_rx_ring);

    // Only meaningful while streaming, where a line held low would otherwise
    // keep re-triggering the interrupt.
    *AT91C_US1_CR = AT91C_US_RSTSTA;
    if (streaming) {
        *AT91C_US1_IER = AT91C_US_RXBRK;
    } else {
        *AT91C_US1_IDR = AT91C_US_RXBRK;
    }

    nx_interrupts_enable(state);

    pbio_serial_port_changed(PBSYS_HOST_TRANSPORT_TYPE_RFCOMM, streaming);
}

//
// Driver process
//

static pbio_os_process_t bt_process;

static pbio_error_t bt_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;
    static pbio_os_timer_t timer;
    static uint8_t args[BT_NAME_SIZE + BT_ADDR_SIZE];
    static pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Revisit: Powering up the BC4 might draw enough current to upset the AVR
    // co-processor and the display, so wait for the AVR link to be established
    // first. This is not definitive, though, but at least empirically the
    // display does not produce garbage this way.
    // PBIO_OS_AWAIT_UNTIL(state, pbdrv_rproc_is_ready());
    PBIO_OS_AWAIT_MS(state, &timer, 1000);
    DEBUG_PRINT("bt start\n");

    // Command mode, chip select asserted, and the BC4 held in reset until the
    // loop releases it.
    *AT91C_PIOA_PER = BT_RST_PIN | BT_ARM_CMD_PIN | BT_CS_PIN;
    *AT91C_PIOA_PPUDR = BT_ARM_CMD_PIN;
    *AT91C_PIOA_SODR = BT_CS_PIN;
    *AT91C_PIOA_CODR = BT_ARM_CMD_PIN | BT_RST_PIN;
    *AT91C_PIOA_OER = BT_RST_PIN | BT_ARM_CMD_PIN | BT_CS_PIN;

    DEBUG_PRINT("pins ok\n");

    while (!bt.closing) {

        // Pulse reset so that the BC4 comes up in a known state, in command
        // mode. The UART is only enabled once it is out of reset, since a
        // receive line held low by the sleeping chip reads as a break.
        *AT91C_PIOA_CODR = BT_RST_PIN | BT_ARM_CMD_PIN;
        bt.streaming = false;
        bt.handle = -1;
        bt.reset_indication = false;
        PBIO_OS_AWAIT_MS(state, &timer, 100);
        *AT91C_PIOA_SODR = BT_RST_PIN;
        bt_uart_init();
        DEBUG_PRINT("uart ok\n");

        pbio_os_timer_set(&timer, BT_RESET_TIMEOUT_MS);
        PBIO_OS_AWAIT_UNTIL(state, bt.reset_indication || pbio_os_timer_is_expired(&timer));
        if (!bt.reset_indication) {
            DEBUG_PRINT("no bc4 e%u\n", bt.checksum_errors);
            PBIO_OS_AWAIT_MS(state, &timer, 1000);
            continue;
        }

        DEBUG_PRINT("bc4 up\n");

        // The USB driver holds off enumeration until this is known, so get it
        // before anything else.
        PBIO_OS_AWAIT(state, &sub, err = bt_command(&sub, BT_MSG_GET_LOCAL_ADDR, NULL, 0, BT_MSG_GET_LOCAL_ADDR_RESULT));
        if (err == PBIO_SUCCESS) {
            memcpy(bt_local_addr, (const void *)bt.reply_args, sizeof(bt_local_addr));
            bt_local_addr_valid = true;
        }

        // The BC4 name is fixed-length and padded with zeros. It is all the
        // host has to go on: the class of device and the service record are
        // burned into the chip firmware.
        memset(args, 0, BT_NAME_SIZE);
        strncpy((char *)args, pbsys_host_get_hub_name(), BT_NAME_SIZE);
        PBIO_OS_AWAIT(state, &sub, err = bt_command(&sub, BT_MSG_SET_FRIENDLY_NAME, args, BT_NAME_SIZE, BT_MSG_SET_FRIENDLY_NAME_ACK));

        if (err == PBIO_SUCCESS) {
            args[0] = true;
            PBIO_OS_AWAIT(state, &sub, err = bt_command(&sub, BT_MSG_SET_DISCOVERABLE, args, 1, BT_MSG_SET_DISCOVERABLE_ACK));
        }

        // Registering the serial port makes the BC4 accept incoming
        // connections on it. It stays open across connections.
        if (err == PBIO_SUCCESS) {
            PBIO_OS_AWAIT(state, &sub, err = bt_command(&sub, BT_MSG_OPEN_PORT, NULL, 0, BT_MSG_PORT_OPEN_RESULT));
        }

        if (err != PBIO_SUCCESS || !bt.reply_args[0]) {
            DEBUG_PRINT("setup fail\n");
            PBIO_OS_AWAIT_MS(state, &timer, 1000);
            continue;
        }

        DEBUG_PRINT("listening\n");

        while (!bt.streaming && !bt.closing) {

            if (bt.pin_requested) {
                DEBUG_PRINT("pin req\n");
                memcpy(args, (const void *)bt.request_addr, BT_ADDR_SIZE);
                memset(&args[BT_ADDR_SIZE], 0, BT_PIN_SIZE);
                strncpy((char *)&args[BT_ADDR_SIZE], BT_PIN_CODE, BT_PIN_SIZE);
                bt.pin_requested = false;
                PBIO_OS_AWAIT(state, &sub, bt_command(&sub, BT_MSG_PIN_CODE, args, BT_ADDR_SIZE + BT_PIN_SIZE, BT_MSG_PIN_CODE_ACK));
                continue;
            }

            if (bt.connection_requested) {
                DEBUG_PRINT("conn req\n");
                args[0] = true;
                bt.connection_requested = false;
                PBIO_OS_AWAIT(state, &sub, bt_command(&sub, BT_MSG_ACCEPT_CONNECTION, args, 1, BT_MSG_NONE));
                continue;
            }

            if (bt.handle >= 0) {
                DEBUG_PRINT("stream h%d\n", bt.handle);
                args[0] = bt.handle;
                PBIO_OS_AWAIT(state, &sub, bt_command(&sub, BT_MSG_OPEN_STREAM, args, 1, BT_MSG_NONE));

                // The BC4 only switches once it has the whole command.
                pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
                PBIO_OS_AWAIT_UNTIL(state, !bt_uart_is_writing() || pbio_os_timer_is_expired(&timer));
                bt_set_streaming(true);
                continue;
            }

            PBIO_OS_AWAIT_MS(state, &timer, 50);
        }

        if (bt.streaming) {
            // The mode line follows the switch with some delay, so wait for it
            // to come up before taking it dropping as a disconnect.
            pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
            PBIO_OS_AWAIT_UNTIL(state, bt_bc4_is_streaming() || pbio_os_timer_is_expired(&timer));
            DEBUG_PRINT("mode %u\n", bt_bc4_is_streaming());

            PBIO_OS_AWAIT_UNTIL(state, !bt_bc4_is_streaming() || bt.closing);

            DEBUG_PRINT("disconn brk%u\n", bt.break_received);
            bt_set_streaming(false);

            // The BC4 reports why it dropped the stream, and does not take
            // commands until it has.
            PBIO_OS_AWAIT_MS(state, &timer, 500);
        }
    }

    // Drop the link explicitly, so that the host sees a disconnect instead of
    // waiting for a timeout.
    if (bt.handle >= 0) {
        args[0] = bt.handle;
        PBIO_OS_AWAIT(state, &sub, bt_command(&sub, BT_MSG_CLOSE_CONNECTION, args, 1, BT_MSG_CLOSE_CONNECTION_RESULT));
    }
    DEBUG_PRINT("closed\n");
    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_init(void) {

    bt.awaiting = BT_MSG_NONE;
    bt.handle = -1;
    lwrb_init(&bt_rx_ring, bt_rx_ring_buf, sizeof(bt_rx_ring_buf));

    // The hardware is claimed from the process instead of here, which runs
    // before the display is up and so cannot report anything.
    pbio_os_process_start(&bt_process, bt_process_thread, NULL);
}

bool pbdrv_bluetooth_nxt_get_local_address(uint8_t *addr) {

    if (!bt_local_addr_valid) {
        return false;
    }

    memcpy(addr, bt_local_addr, sizeof(bt_local_addr));
    return true;
}

//
// Host connection
//

bool pbdrv_bluetooth_classic_host_is_connected(void) {
    return bt.streaming;
}

const char *pbdrv_bluetooth_classic_host_get_connected_name(void) {
    // The BC4 only reports the address of an incoming connection.
    return NULL;
}

void pbdrv_bluetooth_classic_host_disconnect(void) {
    if (bt.closing) {
        return;
    }
    // Closing takes a few exchanges with the BC4, so hold off the shutdown
    // that called this until the process is done.
    bt.closing = true;
    pbio_busy_count_up();
    pbio_os_request_poll();
}

uint32_t pbdrv_bluetooth_classic_host_rx_read(uint8_t *data, uint32_t size) {
    return lwrb_read(&bt_rx_ring, data, size);
}

pbio_error_t pbdrv_bluetooth_classic_host_tx_message(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (!bt.streaming) {
        return PBIO_ERROR_INVALID_OP;
    }

    PBIO_OS_AWAIT_UNTIL(state, !bt_uart_is_writing());
    bt_uart_write(data, size);
    PBIO_OS_AWAIT_UNTIL(state, !bt.streaming || !bt_uart_is_writing());

    if (!bt.streaming) {
        return PBIO_ERROR_INVALID_OP;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

//
// Not implemented yet: inquiry scanning and pairing initiated by the hub, and
// HID devices, which the BC4 firmware does not support at all.
//

pbio_error_t pbdrv_bluetooth_inquiry_start(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

void pbdrv_bluetooth_inquiry_stop(void) {
}

pbio_error_t pbdrv_bluetooth_inquiry_get_results(uint32_t *num, pbio_bluetooth_inquiry_result_t **results) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_classic_host_pair(const uint8_t *bdaddr, const char *name) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_classic_host_pair_status(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

bool pbdrv_bluetooth_classic_host_pair_passkey(uint32_t *passkey) {
    return false;
}

void pbdrv_bluetooth_classic_host_pair_cancel(void) {
}

pbio_error_t pbdrv_bluetooth_classic_hid_pair(const uint8_t *bdaddr, const char *name) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_classic_hid_pair_status(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

void pbdrv_bluetooth_classic_hid_pair_cancel(void) {
}

bool pbdrv_bluetooth_classic_hid_is_connected(void) {
    return false;
}

uint32_t pbdrv_bluetooth_classic_hid_get_report(uint8_t report_id, uint8_t *data, uint32_t size) {
    return 0;
}

bool pbdrv_bluetooth_classic_hid_get_report_id(uint32_t index, uint8_t *report_id) {
    return false;
}

const char *pbdrv_bluetooth_classic_hid_get_connected_name(void) {
    return NULL;
}

void pbdrv_bluetooth_classic_hid_disconnect(void) {
}

#endif // PBDRV_CONFIG_BLUETOOTH_NXT

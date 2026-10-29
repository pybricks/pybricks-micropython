// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Bluetooth driver for the LEGO MINDSTORMS NXT.
//
// The NXT has a CSR BlueCore 4 (BC4) on USART1. This driver talks to the BC4's
// own upper stack (DM, L2CAP, RFCOMM, SDP) over BCSP.
//
// The BC4 flash is never written. At boot, BCCMD over the debug SPI port puts
// the keys for this mode in the BC4's RAM PS store, followed by a warm reset.
// The reset line clears the RAM store, so LEGO's firmware finds the BC4 as it
// left it.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_NXT

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <at91sam7s256.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>
#include <pbdrv/hardware.h>

#include <pbio/bluetooth.h>
#include <pbio/busy_count.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/serial.h>
#include <pbio/util.h>
#include <pbsys/host.h>
#include <pbsys/status.h>

#include <lwrb/lwrb.h>

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

//
// Pins.
//
// Besides the UART, the BC4's debug SPI port is wired to the ARM, sharing
// clock and data out with the display:
//
//   BC4        ARM    Also
//   SPI_CSB    PA31   -
//   SPI_CLK    PA14   display SCL (SPCK)
//   SPI_MOSI   PA13   display SI (MOSI)
//   SPI_MISO   PA12   display A0, through R30 (10k)
//

#define BT_RST_PIN      AT91C_PIO_PA11  // Low holds the BC4 in reset.
#define BT_ARM_CMD_PIN  AT91C_PIO_PA27  // Unused, kept low.
#define BT_SPI_CS_PIN   AT91C_PIO_PA31
#define BT_SPI_CLK_PIN  AT91C_PIO_PA14
#define BT_SPI_MOSI_PIN AT91C_PIO_PA13
#define BT_SPI_MISO_PIN AT91C_PIO_PA12
#define DISPLAY_CS_PIN  AT91C_PIO_PA10

#define BT_UART_PIOA_PINS \
    (AT91C_PA21_RXD1 | AT91C_PA22_TXD1 | AT91C_PA24_RTS1 | AT91C_PA25_CTS1)

/** BCSP baud rate, set in the RAM store before the warm reset. */
#define BT_UART_BAUD (460800)

//
// Debug SPI, bit-banged.
//
// 8-bit command, 16-bit address, then 16-bit words, MSB first. The display is
// deselected around each transfer. Transfers run from the calling process,
// and interrupt handlers do not touch these pins.
//
// The SPI controller belongs to the display driver, which uses PA12 as its A0
// output. Bit-banging only borrows the pins, so the display's controller setup
// and DMA are left alone. This is only used for a few BCCMDs at boot, so speed
// does not matter.
//

#define BT_SPI_HALF_PERIOD (20)
#define BT_SPI_CMD_READ  (0x03)
#define BT_SPI_CMD_WRITE (0x02)

static void bt_spi_delay(void) {
    for (volatile uint32_t i = 0; i < BT_SPI_HALF_PERIOD; i++) {
    }
}

/** Whether the display's DMA transfer may still be using the shared lines. */
static bool bt_spi_display_is_busy(void) {
    if (!(*AT91C_PMC_PCSR & (1 << AT91C_ID_SPI))) {
        return false;
    }
    return *AT91C_SPI_TCR || *AT91C_SPI_TNCR || !(*AT91C_SPI_SR & AT91C_SPI_TXEMPTY);
}

static struct {
    uint32_t pio_enabled;
    uint32_t output_enabled;
    uint32_t output_data;
} bt_spi_saved;

static void bt_spi_claim(void) {
    const uint32_t shared = BT_SPI_CLK_PIN | BT_SPI_MOSI_PIN | BT_SPI_MISO_PIN | DISPLAY_CS_PIN;
    bt_spi_saved.pio_enabled = *AT91C_PIOA_PSR & shared;
    bt_spi_saved.output_enabled = *AT91C_PIOA_OSR & shared;
    bt_spi_saved.output_data = *AT91C_PIOA_ODSR & shared;

    // Deselect the display first, so it ignores everything that follows.
    *AT91C_PIOA_SODR = DISPLAY_CS_PIN;
    *AT91C_PIOA_OER = DISPLAY_CS_PIN;
    *AT91C_PIOA_PER = DISPLAY_CS_PIN;

    *AT91C_PIOA_CODR = BT_SPI_CLK_PIN | BT_SPI_MOSI_PIN;
    *AT91C_PIOA_OER = BT_SPI_CLK_PIN | BT_SPI_MOSI_PIN;
    *AT91C_PIOA_PER = BT_SPI_CLK_PIN | BT_SPI_MOSI_PIN;

    *AT91C_PIOA_ODR = BT_SPI_MISO_PIN;
    *AT91C_PIOA_PER = BT_SPI_MISO_PIN;
}

static void bt_spi_release(void) {
    const uint32_t shared = BT_SPI_CLK_PIN | BT_SPI_MOSI_PIN | BT_SPI_MISO_PIN;

    *AT91C_PIOA_SODR = bt_spi_saved.output_data & shared;
    *AT91C_PIOA_CODR = ~bt_spi_saved.output_data & shared;
    *AT91C_PIOA_OER = bt_spi_saved.output_enabled & shared;
    *AT91C_PIOA_ODR = ~bt_spi_saved.output_enabled & shared;
    *AT91C_PIOA_PDR = ~bt_spi_saved.pio_enabled & shared;

    if (!(bt_spi_saved.output_data & DISPLAY_CS_PIN)) {
        *AT91C_PIOA_CODR = DISPLAY_CS_PIN;
    }
}

static uint32_t bt_spi_shift(uint32_t out, uint32_t bits) {
    uint32_t in = 0;
    for (uint32_t bit = 1 << (bits - 1); bit; bit >>= 1) {
        if (out & bit) {
            *AT91C_PIOA_SODR = BT_SPI_MOSI_PIN;
        } else {
            *AT91C_PIOA_CODR = BT_SPI_MOSI_PIN;
        }
        bt_spi_delay();
        // The BC4 changes MISO on the falling edge, so it is stable by now.
        if (*AT91C_PIOA_PDSR & BT_SPI_MISO_PIN) {
            in |= bit;
        }
        *AT91C_PIOA_SODR = BT_SPI_CLK_PIN;
        bt_spi_delay();
        *AT91C_PIOA_CODR = BT_SPI_CLK_PIN;
    }
    return in;
}

/** Starts a transfer. Two clocks with chip select high reset the BC4's port. */
static void bt_spi_begin(void) {
    *AT91C_PIOA_SODR = BT_SPI_CS_PIN;
    bt_spi_shift(0, 2);
    *AT91C_PIOA_CODR = BT_SPI_CS_PIN;
    bt_spi_delay();
}

static void bt_spi_end(void) {
    bt_spi_delay();
    *AT91C_PIOA_SODR = BT_SPI_CS_PIN;
}

/** Reads XAP data words. Waits for the display, which is brief. */
static pbio_error_t bt_spi_read(uint16_t addr, uint16_t *data, uint32_t size) {
    while (bt_spi_display_is_busy()) {
    }
    bt_spi_claim();
    bt_spi_begin();
    bt_spi_shift(BT_SPI_CMD_READ, 8);
    bt_spi_shift(addr, 16);
    // Reads are acknowledged by an echo of the command and address high byte.
    bool ok = bt_spi_shift(0, 16) == (BT_SPI_CMD_READ << 8 | addr >> 8);
    for (uint32_t i = 0; ok && i < size; i++) {
        data[i] = bt_spi_shift(0, 16);
    }
    bt_spi_end();
    bt_spi_release();
    return ok ? PBIO_SUCCESS : PBIO_ERROR_IO;
}

static uint16_t bt_spi_read_word(uint16_t addr) {
    uint16_t value = 0;
    bt_spi_read(addr, &value, 1);
    return value;
}

static void bt_spi_write(uint16_t addr, const uint16_t *data, uint32_t size) {
    while (bt_spi_display_is_busy()) {
    }
    bt_spi_claim();
    bt_spi_begin();
    bt_spi_shift(BT_SPI_CMD_WRITE, 8);
    bt_spi_shift(addr, 16);
    for (uint32_t i = 0; i < size; i++) {
        bt_spi_shift(data[i], 16);
    }
    bt_spi_end();
    bt_spi_release();
}

//
// BCCMD through the SPI mailbox of the BC4 firmware. Addresses are those of
// stack build 0x0b11, found on every NXT so far.
//

#define BC4_MAILBOX_STATE (0xEE8F)
#define BC4_MAILBOX_ARG (0xEE90)
#define BC4_MAILBOX_BUF (0xEE91)
#define BC4_SPI_USER_CMD (0x006B)

#define BCCMD_GETREQ (0x0000)
#define BCCMD_GETRESP (0x0001)
#define BCCMD_SETREQ (0x0002)
#define BCCMD_VARID_BUILDID (0x2819)
#define BCCMD_VARID_WARM_RESET (0x4002)
#define BCCMD_VARID_PS (0x7003)
#define BCCMD_PS_STORE_RAM (0x0008)
#define BCCMD_PDU_WORDS (9)
#define BCCMD_PDU_WORDS_MAX (10)

/** Most the BC4 may put in one SDP packet. Fits BCSP_PAYLOAD_MAX. */
#define BC4_SDP_PACKET_MAX (540)

/**
 * RAM PS store keys for the on-chip stack on BCSP: key, value size, value.
 * The first two are LEGO's own bootmode 1 settings. ONCHIP_HCI_CLIENT keeps
 * its flash value (1), so the BC4 runs its upper stack.
 */
static const uint16_t bc4_mode_keys[][4] = {
    { 0x04F8, 1, 1 }, // Bootmode 1 VM_DISABLE.
    { 0x04F9, 1, 1 }, // Bootmode 1 HOST_INTERFACE: BCSP.
    { 0x01BE, 1, (uint32_t)BT_UART_BAUD * 4096 / 1000000 }, // UART_BAUDRATE: baud * 0.004096.
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    // HOSTIO_PROTOCOL_INFO10, for BCSP channel 10 (SDP). The first word is the
    // most the BC4 puts in one packet, longer messages come in pieces. It is
    // 256 by default, too little for an SDP answer with a HID descriptor.
    { 0x019E, 2, BC4_SDP_PACKET_MAX, 0xb800 },
    #endif
};

/** Sets the mailbox state, raises its interrupt, and waits for the next one. */
static uint16_t bc4_mailbox_step(uint16_t state) {
    static const uint16_t trigger = 1;
    bt_spi_write(BC4_MAILBOX_STATE, &state, 1);
    bt_spi_write(BC4_SPI_USER_CMD, &trigger, 1);
    uint32_t start = pbdrv_clock_get_us();
    uint16_t now;
    do {
        now = bt_spi_read_word(BC4_MAILBOX_STATE);
    } while ((now == state || now == 5) && pbdrv_clock_get_us() - start < 20000);
    return now;
}

/**
 * Runs one BCCMD PDU through the mailbox. The PDU's length is in its word 1.
 * The response replaces the request, unless @p reply is false (resets do not
 * answer).
 */
static pbio_error_t bc4_bccmd(uint16_t *pdu, bool reply) {
    const uint16_t size = pdu[1];
    if (bt_spi_read_word(BC4_MAILBOX_STATE) != 0) {
        return PBIO_ERROR_BUSY;
    }
    bt_spi_write(BC4_MAILBOX_ARG, &size, 1);
    if (bc4_mailbox_step(1) != 2) {
        return PBIO_ERROR_FAILED;
    }
    uint16_t buf = bt_spi_read_word(BC4_MAILBOX_BUF);
    bt_spi_write(buf, pdu, size);
    if (!reply) {
        static const uint16_t process = 4;
        bt_spi_write(BC4_MAILBOX_STATE, &process, 1);
        bt_spi_write(BC4_SPI_USER_CMD, &process, 1);
        return PBIO_SUCCESS;
    }
    if (bc4_mailbox_step(4) != 6) {
        return PBIO_ERROR_TIMEDOUT;
    }
    bt_spi_read(buf, pdu, size);
    bc4_mailbox_step(7);
    return pdu[0] == BCCMD_GETRESP && pdu[4] == 0 ? PBIO_SUCCESS : PBIO_ERROR_FAILED;
}

//
// UART on USART1: received by the PDC into a ring in two halves, sent by the
// PDC in chunks. Interrupts only wake up the driver process.
//

#define UART_RX_RING_SIZE (256)
static uint8_t uart_rx_ring[UART_RX_RING_SIZE];
static uint32_t uart_rx_read;
/** The half queued as the PDC's next buffer: 0 or 1. */
static uint32_t uart_rx_next_half;

/** Line idle time, in bit periods, after which the interrupt fires. */
#define UART_RX_TIMEOUT_BITS (30)

static uint8_t uart_tx_chunk[64];

static volatile uint32_t uart_rx_errors;

static uint32_t uart_rx_write_index(void) {
    return (*AT91C_US1_RPR - (uint32_t)uart_rx_ring) % UART_RX_RING_SIZE;
}

/** Queues the half the PDC finished again. Call after reading up to it. */
static void uart_rx_requeue(void) {
    if (*AT91C_US1_RNCR == 0) {
        uart_rx_next_half ^= 1;
        *AT91C_US1_RNPR = (uint32_t)&uart_rx_ring[uart_rx_next_half * UART_RX_RING_SIZE / 2];
        *AT91C_US1_RNCR = UART_RX_RING_SIZE / 2;
    }
}

static void uart_isr(void) {
    uint32_t status = *AT91C_US1_CSR;
    if (status & (AT91C_US_FRAME | AT91C_US_PARE | AT91C_US_OVRE)) {
        *AT91C_US1_CR = AT91C_US_RSTSTA;
        uart_rx_errors++;
    }
    if (status & AT91C_US_TIMEOUT) {
        // Wait for the next byte before timing out again.
        *AT91C_US1_CR = AT91C_US_STTTO;
    }
    if (status & AT91C_US_ENDTX) {
        *AT91C_US1_IDR = AT91C_US_ENDTX;
    }
    pbio_os_request_poll();
}

static void uart_open(void) {
    uint32_t state = nx_interrupts_disable();

    *AT91C_PMC_PCER = (1 << AT91C_ID_US1);
    *AT91C_PIOA_PDR = BT_UART_PIOA_PINS;
    *AT91C_PIOA_ASR = BT_UART_PIOA_PINS;

    *AT91C_US1_CR = AT91C_US_TXDIS | AT91C_US_RXDIS;
    *AT91C_US1_IDR = ~0;
    *AT91C_US1_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA | AT91C_US_RSTNACK;

    *AT91C_US1_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;
    *AT91C_US1_RPR = (uint32_t)uart_rx_ring;
    *AT91C_US1_RCR = UART_RX_RING_SIZE / 2;
    *AT91C_US1_RNPR = (uint32_t)&uart_rx_ring[UART_RX_RING_SIZE / 2];
    *AT91C_US1_RNCR = UART_RX_RING_SIZE / 2;
    uart_rx_read = 0;
    uart_rx_next_half = 1;
    *AT91C_US1_TCR = 0;
    *AT91C_US1_TNCR = 0;

    // BCSP is 8E1 without flow control.
    *AT91C_US1_MR = AT91C_US_USMODE_NORMAL |
        AT91C_US_CLKS_CLOCK | AT91C_US_CHRL_8_BITS | AT91C_US_NBSTOP_1_BIT |
        AT91C_US_PAR_EVEN | AT91C_US_CHMODE_NORMAL | AT91C_US_OVER;
    *AT91C_US1_BRGR = (NXT_CLOCK_FREQ / 8 + BT_UART_BAUD / 2) / BT_UART_BAUD;
    *AT91C_US1_RTOR = UART_RX_TIMEOUT_BITS;

    nx_aic_install_isr(AT91C_ID_US1, AIC_PRIO_DRIVER, AIC_TRIG_LEVEL, uart_isr);
    *AT91C_US1_IER = AT91C_US_TIMEOUT | AT91C_US_FRAME | AT91C_US_PARE | AT91C_US_OVRE;

    *AT91C_US1_CR = AT91C_US_TXEN | AT91C_US_RXEN | AT91C_US_RTSEN | AT91C_US_STTTO;
    *AT91C_US1_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;

    nx_interrupts_enable(state);
}

static bool uart_tx_busy(void) {
    return *AT91C_US1_TCR != 0;
}

static void uart_tx_start(uint32_t size) {
    *AT91C_US1_TPR = (uint32_t)uart_tx_chunk;
    *AT91C_US1_TCR = size;
    *AT91C_US1_IER = AT91C_US_ENDTX;
}

//
// BCSP transport (CSR's BlueCore Serial Protocol), as in BlueZ hci_bcsp.c.
//
// Packets are SLIP framed: a 4-byte header (flags with sequence and ack
// numbers, channel and length, header checksum), the payload, and a CRC.
// Reliable packets are acknowledged and resent until they are. This keeps one
// reliable packet in flight.
//

#define BCSP_CHANNEL_ACK (0)
#define BCSP_CHANNEL_LINK (1)
#define BCSP_CHANNEL_BCCMD (2)
#define BCSP_CHANNEL_HQ (3)
#define BCSP_CHANNEL_DM (4)
#define BCSP_CHANNEL_HCI (5)
#define BCSP_CHANNEL_L2CAP (8)
#define BCSP_CHANNEL_RFCOMM (9)
#define BCSP_CHANNEL_SDP (10)

#define BCSP_FLAG_CRC (0x40)
#define BCSP_FLAG_RELIABLE (0x80)

/**
 * Largest payload sent or received. Longer received frames are dropped. Fits
 * an L2CAP data message (7 words) with a whole peer message.
 */
#define BCSP_PAYLOAD_MAX (14 + PBDRV_BLUETOOTH_PEER_MTU + 6)

/** How long to wait for an ack or a link message before sending again. */
#define BCSP_RESEND_MS (250)

/** Resends of one packet after which the link is considered lost. */
#define BCSP_RESENDS_MAX (8)

#define SLIP_END (0xC0)
#define SLIP_ESC (0xDB)
#define SLIP_ESC_END (0xDC)
#define SLIP_ESC_ESC (0xDD)

static const uint8_t bcsp_sync[] = { 0xda, 0xdc, 0xed, 0xed };
static const uint8_t bcsp_sync_resp[] = { 0xac, 0xaf, 0xef, 0xee };
static const uint8_t bcsp_conf[] = { 0xad, 0xef, 0xac, 0xed };
static const uint8_t bcsp_conf_resp[] = { 0xde, 0xad, 0xd0, 0xd0 };

typedef enum {
    BCSP_LINK_OFF,
    BCSP_LINK_SYNC,
    BCSP_LINK_CONF,
    BCSP_LINK_ACTIVE,
} bcsp_link_state_t;

static void bt_handle_message(uint8_t channel, const uint8_t *payload, uint16_t size);

static struct {
    bcsp_link_state_t link;
    /** Our next sequence number. */
    uint8_t txseq;
    /** Next sequence number expected from the BC4, which is also our ack. */
    uint8_t rxack;
    /** Link messages to send: our own SYNC/CONF and responses to the BC4's. */
    const uint8_t *link_message;
    const uint8_t *link_response;
    bool ack_pending;
    /** Times link messages and resends. */
    pbio_os_timer_t timer;
    bool timer_running;
    /** Reliable packet waiting to be sent or acknowledged, header in front. */
    uint8_t reliable[4 + BCSP_PAYLOAD_MAX];
    uint16_t reliable_size;
    bool reliable_pending;
    bool reliable_on_wire;
    /** Frame being encoded: header and payload, then the CRC. */
    const uint8_t *tx_frame;
    uint16_t tx_frame_size;
    uint16_t tx_pos;
    uint8_t tx_crc[2];
    /** Header and payload of the link or ack packet being sent. */
    uint8_t tx_small[4 + 4];
    /** Frame being received: header, payload, CRC. */
    uint8_t rx_frame[4 + BCSP_PAYLOAD_MAX + 2];
    uint16_t rx_size;
    bool rx_escape;
    bool rx_in_frame;
    uint32_t rx_bad;
    uint32_t resends;
    uint32_t resends_in_a_row;
} bcsp;

/** BCSP integrity check: CRC-CCITT, sent bit reversed and big endian. */
static uint16_t bcsp_crc(const uint8_t *data, uint16_t size) {
    static const uint16_t table[16] = {
        0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
        0x8408, 0x9489, 0xa50a, 0xb58b, 0xc60c, 0xd68d, 0xe70e, 0xf78f,
    };
    uint16_t crc = 0xffff;
    for (uint16_t i = 0; i < size; i++) {
        crc = (crc >> 4) ^ table[(crc ^ data[i]) & 0xf];
        crc = (crc >> 4) ^ table[(crc ^ (data[i] >> 4)) & 0xf];
    }
    uint16_t reversed = 0;
    for (int i = 0; i < 16; i++) {
        reversed = reversed << 1 | ((crc >> i) & 1);
    }
    return reversed;
}

static void bcsp_make_header(uint8_t *header, bool reliable, uint8_t channel, uint16_t size) {
    header[0] = BCSP_FLAG_CRC | (reliable ? BCSP_FLAG_RELIABLE | bcsp.txseq : 0) | bcsp.rxack << 3;
    header[1] = (size & 0xf) << 4 | channel;
    header[2] = size >> 4;
    header[3] = ~(header[0] + header[1] + header[2]);
}

static void bcsp_timer_start(void) {
    pbio_os_timer_set(&bcsp.timer, BCSP_RESEND_MS);
    bcsp.timer_running = true;
}

/** Starts sending a frame of header and payload, CRC added on the way. */
static void bcsp_tx_frame(const uint8_t *frame, uint16_t size) {
    // Every packet carries our ack, so any pending one goes with this.
    bcsp.ack_pending = false;
    uint16_t crc = bcsp_crc(frame, size);
    bcsp.tx_crc[0] = crc >> 8;
    bcsp.tx_crc[1] = crc;
    bcsp.tx_frame = frame;
    bcsp.tx_frame_size = size;
    bcsp.tx_pos = 0;
}

/** Encodes and sends the next chunk of the frame being sent. */
static void bcsp_tx_chunk(void) {
    uint32_t total = bcsp.tx_frame_size + 2;
    uint32_t n = 0;
    if (bcsp.tx_pos == 0) {
        uart_tx_chunk[n++] = SLIP_END;
    }
    // Stop two short of the end, since one byte can take two.
    while (bcsp.tx_pos < total && n < sizeof(uart_tx_chunk) - 2) {
        uint8_t b = bcsp.tx_pos < bcsp.tx_frame_size ?
            bcsp.tx_frame[bcsp.tx_pos] : bcsp.tx_crc[bcsp.tx_pos - bcsp.tx_frame_size];
        bcsp.tx_pos++;
        if (b == SLIP_END) {
            uart_tx_chunk[n++] = SLIP_ESC;
            uart_tx_chunk[n++] = SLIP_ESC_END;
        } else if (b == SLIP_ESC) {
            uart_tx_chunk[n++] = SLIP_ESC;
            uart_tx_chunk[n++] = SLIP_ESC_ESC;
        } else {
            uart_tx_chunk[n++] = b;
        }
    }
    if (bcsp.tx_pos == total && n < sizeof(uart_tx_chunk)) {
        uart_tx_chunk[n++] = SLIP_END;
        bcsp.tx_frame = NULL;
    }
    uart_tx_start(n);
}

static void bcsp_tx_small(uint8_t channel, const uint8_t *payload, uint16_t size) {
    bcsp_make_header(bcsp.tx_small, false, channel, size);
    memcpy(&bcsp.tx_small[4], payload, size);
    bcsp_tx_frame(bcsp.tx_small, 4 + size);
}

/** Sends whatever is due next, if the UART is free. */
static void bcsp_tx_run(void) {
    if (uart_tx_busy()) {
        return;
    }
    if (bcsp.tx_frame) {
        bcsp_tx_chunk();
        return;
    }
    if (bcsp.link_response) {
        bcsp_tx_small(BCSP_CHANNEL_LINK, bcsp.link_response, 4);
        bcsp.link_response = NULL;
    } else if (bcsp.link_message) {
        bcsp_tx_small(BCSP_CHANNEL_LINK, bcsp.link_message, 4);
        bcsp.link_message = NULL;
    } else if (bcsp.link == BCSP_LINK_ACTIVE && bcsp.reliable_pending && !bcsp.reliable_on_wire) {
        // The header is redone each time, since our ack may have moved on.
        bcsp_make_header(bcsp.reliable, true, bcsp.reliable[1] & 0xf, bcsp.reliable_size);
        bcsp_tx_frame(bcsp.reliable, 4 + bcsp.reliable_size);
        bcsp.reliable_on_wire = true;
        bcsp_timer_start();
    } else if (bcsp.ack_pending) {
        bcsp_tx_small(BCSP_CHANNEL_ACK, NULL, 0);
    } else {
        return;
    }
    bcsp_tx_chunk();
}

static void bcsp_timer_expired(void) {
    bcsp.timer_running = false;
    switch (bcsp.link) {
        case BCSP_LINK_SYNC:
            bcsp.link_message = bcsp_sync;
            bcsp_timer_start();
            break;
        case BCSP_LINK_CONF:
            bcsp.link_message = bcsp_conf;
            bcsp_timer_start();
            break;
        case BCSP_LINK_ACTIVE:
            if (bcsp.reliable_on_wire) {
                bcsp.reliable_on_wire = false;
                bcsp.resends++;
                if (bcsp.resends_in_a_row == 0) {
                    DEBUG_PRINT("bcsp: resend seq %u ack %u, rx bad %u, uart errors %u\n",
                        bcsp.txseq, bcsp.rxack, bcsp.rx_bad, uart_rx_errors);
                }
                // A BC4 that takes nothing for this long will not recover.
                if (++bcsp.resends_in_a_row > BCSP_RESENDS_MAX) {
                    DEBUG_PRINT("bcsp: no ack for %u resends\n", bcsp.resends_in_a_row);
                    bcsp.link = BCSP_LINK_OFF;
                }
            }
            break;
        default:
            break;
    }
}

static void bcsp_link_start(void) {
    bcsp.link = BCSP_LINK_SYNC;
    bcsp.txseq = 0;
    bcsp.rxack = 0;
    bcsp.reliable_on_wire = false;
    bcsp.link_message = bcsp_sync;
    bcsp_timer_start();
}

static void bcsp_rx_link(const uint8_t *payload) {
    if (memcmp(payload, bcsp_sync, 4) == 0) {
        if (bcsp.link == BCSP_LINK_ACTIVE) {
            // The BC4 restarted. Start over, keeping any unacknowledged
            // packet to send again.
            DEBUG_PRINT("bcsp: peer restarted\n");
            bcsp_link_start();
        }
        bcsp.link_response = bcsp_sync_resp;
    } else if (memcmp(payload, bcsp_conf, 4) == 0) {
        bcsp.link_response = bcsp_conf_resp;
    } else if (memcmp(payload, bcsp_sync_resp, 4) == 0 && bcsp.link == BCSP_LINK_SYNC) {
        bcsp.link = BCSP_LINK_CONF;
        bcsp.link_message = bcsp_conf;
        bcsp_timer_start();
    } else if (memcmp(payload, bcsp_conf_resp, 4) == 0 && bcsp.link == BCSP_LINK_CONF) {
        bcsp.link = BCSP_LINK_ACTIVE;
        bcsp.timer_running = false;
    }
}

static void bcsp_rx_frame(void) {
    uint8_t *header = bcsp.rx_frame;
    uint8_t *payload = header + 4;
    uint16_t size = bcsp.rx_size;

    if (size < 4 || (uint8_t)(header[0] + header[1] + header[2] + header[3]) != 0xff) {
        bcsp.rx_bad++;
        return;
    }
    uint16_t length = header[1] >> 4 | header[2] << 4;
    bool has_crc = header[0] & BCSP_FLAG_CRC;
    if (size != 4 + length + (has_crc ? 2 : 0) ||
        (has_crc && (payload[length] << 8 | payload[length + 1]) != bcsp_crc(header, 4 + length))) {
        bcsp.rx_bad++;
        return;
    }

    uint8_t channel = header[1] & 0xf;
    if (channel == BCSP_CHANNEL_LINK) {
        bcsp_rx_link(payload);
        return;
    }
    if (bcsp.link != BCSP_LINK_ACTIVE) {
        return;
    }

    // Our reliable packet is acknowledged once the ack moves past it.
    uint8_t ack = header[0] >> 3 & 7;
    if (bcsp.reliable_on_wire && ack == ((bcsp.txseq + 1) & 7)) {
        bcsp.resends_in_a_row = 0;
        bcsp.timer_running = false;
        bcsp.reliable_pending = false;
        bcsp.reliable_on_wire = false;
        bcsp.txseq = (bcsp.txseq + 1) & 7;
    }

    if (!(header[0] & BCSP_FLAG_RELIABLE)) {
        return;
    }

    // Ack everything reliable, but take only the one expected next. Others
    // are resends of what we already have, or out of order.
    bcsp.ack_pending = true;
    if ((header[0] & 7) != bcsp.rxack) {
        return;
    }
    bcsp.rxack = (bcsp.rxack + 1) & 7;

    bt_handle_message(channel, payload, length);
}

static void bcsp_rx_run(void) {
    uint32_t write;
    while ((write = uart_rx_write_index()) != uart_rx_read) {
        const uint8_t *block = &uart_rx_ring[uart_rx_read];
        uint32_t length = (write > uart_rx_read ? write : UART_RX_RING_SIZE) - uart_rx_read;
        for (uint32_t i = 0; i < length; i++) {
            uint8_t b = block[i];
            if (b == SLIP_END) {
                if (bcsp.rx_in_frame && bcsp.rx_size) {
                    bcsp_rx_frame();
                }
                bcsp.rx_in_frame = true;
                bcsp.rx_size = 0;
                bcsp.rx_escape = false;
                continue;
            }
            if (!bcsp.rx_in_frame) {
                continue;
            }
            if (bcsp.rx_escape) {
                b = b == SLIP_ESC_END ? SLIP_END : SLIP_ESC;
                bcsp.rx_escape = false;
            } else if (b == SLIP_ESC) {
                bcsp.rx_escape = true;
                continue;
            }
            if (bcsp.rx_size >= sizeof(bcsp.rx_frame)) {
                // Too long, so drop it and wait for the next frame.
                bcsp.rx_in_frame = false;
                bcsp.rx_bad++;
                continue;
            }
            bcsp.rx_frame[bcsp.rx_size++] = b;
        }
        uart_rx_read = (uart_rx_read + length) % UART_RX_RING_SIZE;
    }
    uart_rx_requeue();
}

/** Processes received data, timers and sending. Called on every poll. */
static void bcsp_poll(void) {
    if (bcsp.link == BCSP_LINK_OFF) {
        return;
    }
    bcsp_rx_run();
    if (bcsp.timer_running && pbio_os_timer_is_expired(&bcsp.timer)) {
        bcsp_timer_expired();
    }
    bcsp_tx_run();
}

static bool bcsp_can_send(void) {
    return bcsp.link == BCSP_LINK_ACTIVE && !bcsp.reliable_pending;
}

/** Buffer for the payload of the next reliable packet, BCSP_PAYLOAD_MAX long. */
static uint8_t *bcsp_payload(void) {
    return &bcsp.reliable[4];
}

/**
 * Queues the reliable packet put together in bcsp_payload(). Only call when
 * bcsp_can_send() is true.
 */
static void bcsp_send(uint8_t channel, uint16_t size) {
    bcsp.reliable[1] = channel;
    bcsp.reliable_size = size;
    bcsp.reliable_pending = true;
    pbio_os_request_poll();
}

//
// BlueStack messages.
//
// Messages are 16-bit words, sent little endian: the type, then its fields.
//

/** Phandle that we register with: bit 15 routes upward messages to the host. */
#define BT_PHANDLE (0x8000)

// Device manager (DM), BCSP channel 4. Numbers as in BlueStack's dm_prim.h.
#define DM_AM_REGISTER_REQ (0x0000)
#define DM_AM_REGISTER_CFM (0x0001)

// HCI wrappers. Requests use the HCI opcode as type, followed by their length.
// Results have their own numbers in the same group.
#define DM_HCI_CHANGE_LOCAL_NAME (0x0c13)
#define DM_HCI_WRITE_SCAN_ENABLE (0x0c1a)
#define DM_HCI_WRITE_CLASS_OF_DEVICE (0x0c24)
#define DM_HCI_READ_BD_ADDR (0x1009)
#define DM_HCI_READ_BD_ADDR_COMPLETE (0x100e)
#define DM_HCI_INQUIRY (0x0401)
#define DM_HCI_INQUIRY_CANCEL (0x0402)
#define DM_HCI_REMOTE_NAME_REQUEST (0x0419)
#define DM_HCI_INQUIRY_CANCEL_COMPLETE (0x0421)
#define DM_HCI_INQUIRY_RESULT (0x0424)
#define DM_HCI_INQUIRY_COMPLETE (0x0425)
#define DM_HCI_REMOTE_NAME_COMPLETE (0x0429)

// Link policy. Requests that name a link have a handle word, which the DM
// fills in, before the address. Switch role names it by address only.
#define DM_HCI_SNIFF_MODE (0x0803)
#define DM_HCI_ROLE_DISCOVERY (0x0809)
#define DM_HCI_SWITCH_ROLE (0x080b)
#define DM_HCI_MODE_CHANGE_EVENT (0x080f)
#define DM_HCI_ROLE_DISCOVERY_COMPLETE (0x0812)
#define DM_HCI_SWITCH_ROLE_COMPLETE (0x0813)

// Security manager.
#define DM_SM_LINK_KEY_REQUEST_RES (0x2c0a)
#define DM_SM_PIN_REQUEST_RES (0x2c0b)
#define DM_SM_AUTHORISE_RES (0x2c0c)
#define DM_SM_AUTHENTICATE_REQ (0x2c0d)
#define DM_SM_LINK_KEY_REQUEST_IND (0x2c16)
#define DM_SM_PIN_REQUEST_IND (0x2c17)
#define DM_SM_LINK_KEY_IND (0x2c18)
#define DM_SM_AUTHORISE_IND (0x2c19)
#define DM_SM_AUTHENTICATE_CFM (0x2c1a)

// Private DM messages.
#define DM_SET_DEFAULT_LINK_POLICY (0x2804)
#define DM_ACL_CLOSE_REQ (0x2806)
#define DM_ACL_OPENED_IND (0x280d)
#define DM_ACL_CLOSED_IND (0x280e)

/** Link policy settings: role switch and sniff allowed. */
#define BT_LINK_POLICY (0x0005)

/** PIN for legacy pairing, which is all the BC4 has. Same as LEGO's firmware. */
#define BT_PIN_CODE "1234"

/** Name of a host in the bonding store until we know better. */
#define BT_HOST_DEFAULT_NAME "Computer"

// RFCOMM, BCSP channel 9. Numbers as in BlueStack's rfcomm_prim.h.
#define RFC_INIT_REQ (0x0001)
#define RFC_INIT_CFM (0x0002)
#define RFC_REGISTER_REQ (0x0003)
#define RFC_REGISTER_CFM (0x0004)
#define RFC_START_RES (0x0006)
#define RFC_START_IND (0x0007)
#define RFC_CLOSE_REQ (0x000a)
#define RFC_CLOSE_IND (0x000b)
#define RFC_ESTABLISH_RES (0x000d)
#define RFC_ESTABLISH_IND (0x000e)
#define RFC_RELEASE_REQ (0x0010)
#define RFC_RELEASE_IND (0x0011)
#define RFC_DATA_REQ (0x0018)
#define RFC_DATA_IND (0x0019)
#define RFC_DATAWRITE_REQ (0x001a)
#define RFC_DATAWRITE_CFM (0x001b)
#define RFC_CONTROL_REQ (0x001c)
#define RFC_PARNEG_RES (0x0021)
#define RFC_PARNEG_IND (0x0022)

/** L2CAP PSM of RFCOMM. */
#define BT_PSM_RFCOMM (0x0003)

/** RFCOMM modem signals we report: ready to communicate, ready to receive, data valid. */
#define RFC_MODEM_SIGNALS (0x0b)

// SDP server, BCSP channel 10.
#define SDS_REGISTER_REQ (0x000e)
#define SDS_REGISTER_CFM (0x000f)

// SDP client, also on channel 10.
#define SDC_SERVICE_SEARCH_ATTRIBUTE_REQ (0x0005)
#define SDC_SERVICE_SEARCH_ATTRIBUTE_CFM (0x0006)

// L2CAP, BCSP channel 8.
#define L2CA_REGISTER_REQ (0x0001)
#define L2CA_REGISTER_CFM (0x0002)
#define L2CA_CONNECT_REQ (0x0003)
#define L2CA_CONNECT_RSP (0x0004)
#define L2CA_CONNECT_IND (0x0005)
#define L2CA_CONNECT_CFM (0x0006)
#define L2CA_CONFIG_REQ (0x0007)
#define L2CA_CONFIG_RSP (0x0008)
#define L2CA_CONFIG_IND (0x0009)
#define L2CA_CONFIG_CFM (0x000a)
#define L2CA_DATAWRITE_REQ (0x000b)
#define L2CA_DATAWRITE_CFM (0x000c)
#define L2CA_DATAREAD_IND (0x000d)
#define L2CA_DISCONNECT_REQ (0x000e)
#define L2CA_DISCONNECT_RSP (0x000f)
#define L2CA_DISCONNECT_IND (0x0010)
#define L2CA_DISCONNECT_CFM (0x0011)

// L2CAP connect results.
#define L2CA_CONNECT_PENDING (0x0001)
#define L2CA_CONNECT_REFUSED (0x0004)

/** L2CAP PSMs of HID and of the hub network. */
#define BT_PSM_HID_CONTROL (0x0011)
#define BT_PSM_HID_INTERRUPT (0x0013)
#define BT_PSM_PEER (0x1001)

/** MTU we take on every L2CAP channel. */
#define BT_L2CAP_MTU (PBDRV_BLUETOOTH_PEER_MTU)

/** HCI scan enable: page scan only, or inquiry and page scan. */
#define BT_SCAN_PAGE (2)
#define BT_SCAN_INQUIRY_AND_PAGE (3)

// Gamepad, as on EV3. Toy robot (LEGO's choice) is hidden by Windows 11, and
// computer and phone classes by ChromeOS.
#define BT_CLASS_OF_DEVICE (0x000508)

/** Largest RFCOMM frame we accept, so a data message fits BCSP_PAYLOAD_MAX. */
#define BT_RFCOMM_FRAME_MAX (127)

/** How long sending to the host may stall before the connection is dropped. */
#define BT_TX_TIMEOUT_MS (2000)

/**
 * Most frames handed to the BC4 that it has not yet sent on. It keeps them in
 * its own memory until then, and stops altogether when that runs out.
 */
#define BT_TX_UNCONFIRMED_MAX (2)

/** The same for L2CAP packets. */
#define BT_L2CAP_UNCONFIRMED_MAX (2)

/** Whether to accept credit based flow control when the host asks for it. */
#define BT_RFCOMM_CREDIT_FLOW (1)

/** Most RFCOMM frames we let the host send ahead of reading them. */
#define BT_RFCOMM_CREDITS_MAX (7)

/** Longest message the receive handler queues as a reply, in words. */
#define BT_REPLY_WORDS_MAX (22)

/** How many replies can wait for the link. */
#define BT_REPLY_QUEUE_SIZE (6)

/** Words kept of the message the process waits for. */
#define BT_AWAITED_WORDS_MAX (8)

/** Received RFCOMM data, until the host protocol reads it. */
static lwrb_t bt_rx_ring;
static uint8_t bt_rx_ring_buf[512];

typedef struct {
    uint8_t channel;
    uint8_t size;
    uint16_t words[BT_REPLY_WORDS_MAX];
} bt_reply_t;

static struct {
    /** Message the process waits for, and its first words once here. */
    uint8_t awaited_channel;
    uint16_t awaited;
    bool received;
    uint16_t message[BT_AWAITED_WORDS_MAX];
    /** Whether the BC4 is up and takes connections. */
    bool ready;
    /** Whether we answer inquiries, and whether the BC4 is still to be told. */
    bool discoverable;
    bool scan_enable_due;
    /** Whether a remote name request is running. */
    bool name_busy;
    /** Host whose name is to be asked, since it paired with a placeholder. */
    bool host_name_due;
    uint8_t host_name_addr[6];
    /** Devices that completed pairing since boot, and the most recent one. */
    uint32_t pair_count;
    uint8_t paired_addr[6];
    /** Address of the host, most significant byte first. */
    uint8_t host_addr[6];
    /** Replies from the receive handler, sent by the process. */
    bt_reply_t replies[BT_REPLY_QUEUE_SIZE];
    uint8_t reply_first;
    uint8_t reply_count;
    /** Our RFCOMM server channel, from the BC4. */
    uint8_t server_chan;
    /** The host connection. */
    bool connected;
    bool disconnect_requested;
    /** Whether the host's multiplexer is open, which outlives its channel. */
    bool mux_open;
    uint16_t mux_id;
    uint16_t frame_size;
    bool credit_flow;
    /** Frames the host lets us send. */
    uint16_t tx_credits;
    /** Frames we let the host send. */
    uint16_t rx_credits;
    /** Frames sent to the BC4 that it has not confirmed. */
    uint16_t tx_unconfirmed;
    /** L2CAP packets sent to the BC4 that it has not confirmed. */
    uint16_t l2cap_unconfirmed;
    /** Our own address, most significant byte first. */
    uint8_t local_addr[6];
    /** Links to other devices that are up. */
    uint8_t acl_count;
    /** Whether to close everything and turn the BC4 off. */
    bool shutdown_requested;
} bt;

static uint16_t bt_get_word(const uint8_t *payload, uint16_t index) {
    return pbio_get_uint16_le(&payload[index * 2]);
}

/** Converts the BD_ADDR_T words of a message (lap high, lap low, uap, nap) to an address, most significant byte first. */
static void bt_addr_from_words(const uint16_t *words, uint8_t *addr) {
    addr[0] = words[3] >> 8;
    addr[1] = words[3];
    addr[2] = words[2];
    addr[3] = words[0];
    addr[4] = words[1] >> 8;
    addr[5] = words[1];
}

/** Converts an address, most significant byte first, to BD_ADDR_T words. */
static void bt_addr_to_words(const uint8_t *addr, uint16_t *words) {
    words[0] = addr[3];
    words[1] = addr[4] << 8 | addr[5];
    words[2] = addr[2];
    words[3] = addr[0] << 8 | addr[1];
}

/** Puts words at @p buf, returning the number of bytes. */
static uint16_t bt_put_words(uint8_t *buf, const uint16_t *words, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        pbio_set_uint16_le(&buf[i * 2], words[i]);
    }
    return count * 2;
}

/** Sends a message to the BC4. Only call when bcsp_can_send() is true. */
static void bt_send_message(uint8_t channel, const uint16_t *words, uint16_t count) {
    bcsp_send(channel, bt_put_words(bcsp_payload(), words, count));
}

/**
 * Sends a message whose last field points to @p data. The pointer goes as a
 * zero and the data follows the message, two bytes per word. Only call when
 * bcsp_can_send() is true.
 */
static void bt_send_message_with_data(uint8_t channel, const uint16_t *words, uint16_t count, const uint8_t *data, uint16_t size) {
    uint8_t *buf = bcsp_payload();
    uint16_t n = bt_put_words(buf, words, count);
    memcpy(&buf[n], data, size);
    bcsp_send(channel, n + size);
}

/** Arms the wait for message @p type on @p channel. */
static void bt_await(uint8_t channel, uint16_t type) {
    bt.awaited_channel = channel;
    bt.awaited = type;
    bt.received = false;
}

/** Queues a reply from the receive handler, for the process to send. */
static void bt_reply(uint8_t channel, const uint16_t *words, uint8_t count) {
    if (bt.reply_count == BT_REPLY_QUEUE_SIZE) {
        DEBUG_PRINT("bt: reply queue full, dropped %04x\n", words[0]);
        return;
    }
    bt_reply_t *reply = &bt.replies[(bt.reply_first + bt.reply_count) % BT_REPLY_QUEUE_SIZE];
    reply->channel = channel;
    reply->size = count;
    memcpy(reply->words, words, count * sizeof(uint16_t));
    bt.reply_count++;
    pbio_os_request_poll();
}

/**
 * How many more frames the host may send, given the room left to keep them.
 * Adds them to what the host has.
 */
static uint16_t bt_take_credits_to_give(void) {
    if (!bt.connected || !bt.credit_flow) {
        return 0;
    }
    uint16_t room = lwrb_get_free(&bt_rx_ring) / bt.frame_size;
    if (room > BT_RFCOMM_CREDITS_MAX) {
        room = BT_RFCOMM_CREDITS_MAX;
    }
    uint16_t credits = room > bt.rx_credits ? room - bt.rx_credits : 0;
    bt.rx_credits += credits;
    return credits;
}

static void bt_connection_reset(void) {
    if (bt.connected) {
        pbio_serial_port_changed(PBSYS_HOST_TRANSPORT_TYPE_RFCOMM, false);
    }
    bt.connected = false;
    bt.disconnect_requested = false;
    bt.tx_credits = 0;
    bt.rx_credits = 0;
    bt.tx_unconfirmed = 0;
    lwrb_reset(&bt_rx_ring);
}

static void bt_handle_rfcomm(uint16_t type, const uint8_t *payload, uint16_t size) {
    switch (type) {
        case RFC_REGISTER_CFM:
            bt.server_chan = bt_get_word(payload, 2);
            break;
        case RFC_START_IND: {
            // Words: type, phandle, address (4), mux, port speed, frame size.
            // The host opens the multiplexer. Accept with its parameters.
            uint16_t words[4];
            for (uint16_t i = 0; i < 4; i++) {
                words[i] = bt_get_word(payload, 2 + i);
            }
            bt_addr_from_words(words, bt.host_addr);
            bt.mux_id = bt_get_word(payload, 6);
            bt.mux_open = true;
            const uint16_t reply[] = {
                RFC_START_RES, bt_get_word(payload, 6), true,
                bt_get_word(payload, 7), bt_get_word(payload, 8), BT_PHANDLE,
            };
            bt_reply(BCSP_CHANNEL_RFCOMM, reply, PBIO_ARRAY_SIZE(reply));
            break;
        }
        case RFC_PARNEG_IND: {
            // Words: type, phandle, mux, channel, frame size, credit flow
            // control, initial credits for us.
            bt_connection_reset();
            bt.mux_id = bt_get_word(payload, 2);
            bt.frame_size = bt_get_word(payload, 4);
            if (bt.frame_size > BT_RFCOMM_FRAME_MAX || bt.frame_size == 0) {
                bt.frame_size = BT_RFCOMM_FRAME_MAX;
            }
            bt.credit_flow = BT_RFCOMM_CREDIT_FLOW && bt_get_word(payload, 5);
            bt.tx_credits = bt_get_word(payload, 6);
            // Counted as given once connected.
            bt.connected = true;
            uint16_t credits = bt_take_credits_to_give();
            bt.connected = false;
            const uint16_t reply[] = {
                RFC_PARNEG_RES, bt.mux_id, bt_get_word(payload, 3),
                bt.frame_size, bt.credit_flow, credits,
            };
            bt_reply(BCSP_CHANNEL_RFCOMM, reply, PBIO_ARRAY_SIZE(reply));
            break;
        }
        case RFC_ESTABLISH_IND: {
            // Words: type, phandle, mux, channel.
            const uint16_t reply[] = { RFC_ESTABLISH_RES, bt_get_word(payload, 2), bt_get_word(payload, 3), true };
            bt_reply(BCSP_CHANNEL_RFCOMM, reply, PBIO_ARRAY_SIZE(reply));
            const uint16_t control[] = { RFC_CONTROL_REQ, bt_get_word(payload, 2), bt_get_word(payload, 3), RFC_MODEM_SIGNALS, 0 };
            bt_reply(BCSP_CHANNEL_RFCOMM, control, PBIO_ARRAY_SIZE(control));
            bt.mux_id = bt_get_word(payload, 2);
            bt.connected = true;
            pbio_serial_port_changed(PBSYS_HOST_TRANSPORT_TYPE_RFCOMM, true);
            break;
        }
        case RFC_DATA_IND: {
            // Words: type, phandle, mux, channel, credits, length, payload
            // pointer, release pointer, then the data.
            if (size < 16) {
                break;
            }
            bt.tx_credits += bt_get_word(payload, 4);
            uint16_t length = bt_get_word(payload, 5);
            if (length == 0) {
                break;
            }
            if (bt.rx_credits) {
                bt.rx_credits--;
            }
            // The pointers hold the BC4's own addresses. The data follows.
            if (size - 16 >= length) {
                lwrb_write(&bt_rx_ring, &payload[16], length);
            }
            DEBUG_PRINT("bt: data %u bytes, credits tx %u rx %u\n", length, bt.tx_credits, bt.rx_credits);
            break;
        }
        case RFC_DATAWRITE_CFM:
            if (bt.tx_unconfirmed) {
                bt.tx_unconfirmed--;
            }
            break;
        case RFC_CLOSE_IND:
            bt.mux_open = false;
            bt_connection_reset();
            break;
        case RFC_RELEASE_IND:
            bt_connection_reset();
            break;
        default:
            break;
    }
}

/**
 * Device that is pairing for the first time. Its bonding record is dropped
 * if its link closes before a key comes.
 */
static uint8_t bt_provisional_addr[6];

/** Asks a device its name. Only call when bcsp_can_send() is true. */
static void bt_remote_name_request(const uint8_t *addr) {
    // Words: type, length, address (4), page scan repetition mode, page scan
    // mode, clock offset. The DM fills in the last three.
    uint16_t msg[9] = { DM_HCI_REMOTE_NAME_REQUEST, 9 };
    bt_addr_to_words(addr, &msg[2]);
    bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
    bt.name_busy = true;
}

#if PBDRV_CONFIG_BLUETOOTH_INQUIRY

//
// Inquiry, to find gamepads to pair with.
//
// The BC4 predates extended inquiry results, so names do not come with the
// inquiry, and each device has to be asked. Names are asked between
// inquiries, so an inquiry is cut short when a device is found, and started
// again once its name is in. While the BC4 inquires, other bricks could not
// connect to it, so there is a pause between inquiries to let bonded devices
// connect.
//
// Only devices of the gamepad class are listed, since asking every device
// takes too long. Pybricks hubs use this class too, so they are listed as
// well.
//

/** How long a scan lasts, as on EV3. Callers start another to keep looking. */
#define INQUIRY_SCAN_MS (60000)

/** Length of one inquiry, in units of 1.28 s. */
#define INQUIRY_LENGTH (8)

/** Pause between inquiries. */
#define INQUIRY_PAUSE_MS (1000)

/** Class of device of a gamepad, major and minor class only. */
#define INQUIRY_CLASS_GAMEPAD (0x0508)
#define INQUIRY_CLASS_MASK (0x1ffc)

static struct {
    /** Whether a scan is on, until stopped or its time is up. */
    bool busy;
    /** Whether an inquiry runs, and whether it is being cancelled. */
    bool running;
    bool cancelling;
    /** Ends the scan. */
    pbio_os_timer_t scan_timer;
    /** Pause before the next inquiry. */
    pbio_os_timer_t pause_timer;
    /** Devices found and named. */
    uint32_t count;
    /** Devices found whose names are still to be asked, after the others. */
    uint32_t unnamed;
    pbio_bluetooth_inquiry_result_t results[PBDRV_BLUETOOTH_INQUIRY_NUM_RESULTS];
} inquiry;

/** Whether a scan is on, ending it once its time is up. */
static bool inquiry_is_busy(void) {
    if (inquiry.busy && (bt.shutdown_requested || pbio_os_timer_is_expired(&inquiry.scan_timer))) {
        pbdrv_bluetooth_inquiry_stop();
    }
    return inquiry.busy;
}

static bool inquiry_is_known(const uint8_t *addr) {
    for (uint32_t i = 0; i < inquiry.count + inquiry.unnamed; i++) {
        if (memcmp(inquiry.results[i].bdaddr, addr, 6) == 0) {
            return true;
        }
    }
    return false;
}

static void inquiry_handle_dm(uint16_t type, const uint8_t *payload, uint16_t size) {
    switch (type) {
        case DM_HCI_INQUIRY_RESULT: {
            // Words: type, phandle, count, pointers, then the results at the
            // end, 10 words each: address (4), page scan repetition mode,
            // period mode and mode, class of device (2), clock offset.
            uint16_t count = bt_get_word(payload, 2);
            uint16_t first = size / 2 - count * 10;
            if (!inquiry.busy || !inquiry.running || count == 0 || size / 2 < 3 + count * 10) {
                break;
            }
            for (uint16_t r = 0; r < count; r++) {
                uint16_t w = first + r * 10;
                uint32_t cod = bt_get_word(payload, w + 7) << 16 | bt_get_word(payload, w + 8);
                uint16_t words[4];
                uint8_t addr[6];
                for (uint16_t i = 0; i < 4; i++) {
                    words[i] = bt_get_word(payload, w + i);
                }
                bt_addr_from_words(words, addr);
                DEBUG_PRINT("bt: found %02x:%02x:%02x:%02x:%02x:%02x class %06x\n",
                    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], cod);
                if ((cod & INQUIRY_CLASS_MASK) != INQUIRY_CLASS_GAMEPAD ||
                    inquiry.count + inquiry.unnamed == PBDRV_BLUETOOTH_INQUIRY_NUM_RESULTS || inquiry_is_known(addr)) {
                    continue;
                }
                pbio_bluetooth_inquiry_result_t *result = &inquiry.results[inquiry.count + inquiry.unnamed++];
                memset(result, 0, sizeof(*result));
                memcpy(result->bdaddr, addr, sizeof(addr));
                result->class_of_device = cod;
                pbio_os_request_poll();
            }
            break;
        }
        case DM_HCI_INQUIRY_COMPLETE:
        case DM_HCI_INQUIRY_CANCEL_COMPLETE:
            if (!inquiry.running) {
                break;
            }
            inquiry.running = false;
            inquiry.cancelling = false;
            // Ask the names right away, or pause before the next inquiry.
            pbio_os_timer_set(&inquiry.pause_timer, inquiry.unnamed ? 0 : INQUIRY_PAUSE_MS);
            pbio_os_request_poll();
            break;
        default:
            break;
    }
}

/** Takes a name answer, which may be for the next device to be named. */
static void inquiry_handle_name(const uint8_t *addr, const char *name) {
    pbio_bluetooth_inquiry_result_t *result = &inquiry.results[inquiry.count];
    if (!inquiry.unnamed || memcmp(result->bdaddr, addr, 6) != 0) {
        return;
    }
    // Listed even without a name, so that it is not asked forever.
    snprintf(result->name, sizeof(result->name), "%s", name);
    inquiry.count++;
    inquiry.unnamed--;
}

/** Sends what the scan needs next, if anything. */
static bool inquiry_send_next(void) {
    bool busy = inquiry_is_busy();

    // Stop the inquiry once the scan is over, or when a name is to be asked.
    if (inquiry.running && !inquiry.cancelling && (!busy || inquiry.unnamed || bt.host_name_due)) {
        static const uint16_t msg[] = { DM_HCI_INQUIRY_CANCEL, 2 };
        inquiry.cancelling = true;
        bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
        return true;
    }
    if (!busy || inquiry.running || bt.name_busy || bt.host_name_due) {
        return false;
    }
    if (inquiry.unnamed) {
        bt_remote_name_request(inquiry.results[inquiry.count].bdaddr);
        return true;
    }
    if (!pbio_os_timer_is_expired(&inquiry.pause_timer)) {
        return false;
    }
    // Words: type, length, LAP (general inquiry, 2), length, responses (0:
    // any number).
    static const uint16_t msg[] = { DM_HCI_INQUIRY, 6, 0x009e, 0x8b33, INQUIRY_LENGTH, 0 };
    inquiry.running = true;
    bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
    DEBUG_PRINT("bt: inquiry\n");
    return true;
}

/** Forgets the scan, as after a BC4 restart. */
static void inquiry_reset(void) {
    inquiry.busy = false;
    inquiry.running = false;
    inquiry.cancelling = false;
    inquiry.unnamed = 0;
}

pbio_error_t pbdrv_bluetooth_inquiry_start(void) {
    if (!bt.ready || bt.shutdown_requested) {
        return PBIO_ERROR_INVALID_OP;
    }
    if (inquiry_is_busy()) {
        return PBIO_ERROR_BUSY;
    }
    // An inquiry left over from the last scan is fine to keep.
    inquiry.busy = true;
    inquiry.count = 0;
    inquiry.unnamed = 0;
    pbio_os_timer_set(&inquiry.scan_timer, INQUIRY_SCAN_MS);
    pbio_os_timer_set(&inquiry.pause_timer, 0);
    pbio_os_request_poll();
    return PBIO_SUCCESS;
}

void pbdrv_bluetooth_inquiry_stop(void) {
    if (!inquiry.busy) {
        return;
    }
    // Any inquiry is cancelled by the process. Names still to come are
    // ignored.
    inquiry.busy = false;
    inquiry.unnamed = 0;
    pbio_os_request_poll();
}

pbio_error_t pbdrv_bluetooth_inquiry_get_results(uint32_t *num, pbio_bluetooth_inquiry_result_t **results) {
    if (!inquiry_is_busy()) {
        return PBIO_ERROR_INVALID_OP;
    }
    *num = inquiry.count;
    *results = inquiry.results;
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BLUETOOTH_INQUIRY

/** Whether an inquiry runs, during which names cannot be asked. */
static bool bt_inquiry_is_running(void) {
    #if PBDRV_CONFIG_BLUETOOTH_INQUIRY
    return inquiry.running;
    #else
    return false;
    #endif
}

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
static const char *hid_pin_code(const uint8_t *bdaddr);
static void hid_handle_dm(uint16_t type, const uint8_t *payload, uint16_t size);
#endif

static void bt_handle_dm(uint16_t type, const uint8_t *payload, uint16_t size) {
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    hid_handle_dm(type, payload, size);
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_INQUIRY
    inquiry_handle_dm(type, payload, size);
    #endif

    // Address words, as they come after type and phandle in indications.
    uint16_t addr[4];
    for (uint16_t i = 0; i < 4; i++) {
        addr[i] = size >= 12 ? bt_get_word(payload, 2 + i) : 0;
    }
    uint8_t bdaddr[6];
    bt_addr_from_words(addr, bdaddr);

    switch (type) {
        case DM_SM_LINK_KEY_REQUEST_IND: {
            // Words of the reply: type, address (4), key (16, a byte each),
            // valid. Without a stored key, the host has to pair again.
            uint16_t reply[22] = { DM_SM_LINK_KEY_REQUEST_RES, addr[0], addr[1], addr[2], addr[3] };
            uint8_t key[16];
            uint16_t key_type;
            if (pbio_bluetooth_classic_link_key_get(bdaddr, key, &key_type)) {
                for (uint16_t i = 0; i < 16; i++) {
                    reply[5 + i] = key[i];
                }
                reply[21] = true;
            }
            DEBUG_PRINT("bt: link key request, %s\n", reply[21] ? "known" : "unknown");
            bt_reply(BCSP_CHANNEL_DM, reply, PBIO_ARRAY_SIZE(reply));
            break;
        }
        case DM_SM_PIN_REQUEST_IND: {
            // Pairing starts. Only registered devices get their key stored,
            // so register one we do not know yet, as EV3 does.
            if (!pbio_bluetooth_classic_link_key_get_name(bdaddr)) {
                pbio_bluetooth_classic_link_key_register(bdaddr, BT_HOST_DEFAULT_NAME);
                memcpy(bt_provisional_addr, bdaddr, sizeof(bt_provisional_addr));
            }
            const char *pin = BT_PIN_CODE;
            #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
            pin = hid_pin_code(bdaddr);
            #endif
            // Words of the reply: type, address (4), PIN length, PIN (16, a
            // byte each).
            uint16_t reply[22] = { DM_SM_PIN_REQUEST_RES, addr[0], addr[1], addr[2], addr[3], strlen(pin) };
            for (uint16_t i = 0; pin[i]; i++) {
                reply[6 + i] = pin[i];
            }
            DEBUG_PRINT("bt: PIN request, answered %s\n", pin);
            bt_reply(BCSP_CHANNEL_DM, reply, PBIO_ARRAY_SIZE(reply));
            break;
        }
        case DM_SM_LINK_KEY_IND: {
            // Words: type, phandle, address (4), key type, key (16, a byte each).
            if (size < 22 * 2) {
                break;
            }
            uint8_t key[16];
            for (uint16_t i = 0; i < 16; i++) {
                key[i] = bt_get_word(payload, 7 + i);
            }
            pbio_bluetooth_classic_link_key_put(bdaddr, key, bt_get_word(payload, 6));
            if (memcmp(bdaddr, bt_provisional_addr, sizeof(bdaddr)) == 0) {
                memset(bt_provisional_addr, 0, sizeof(bt_provisional_addr));
            }
            DEBUG_PRINT("bt: paired, key type %u\n", bt_get_word(payload, 6));
            // Gamepads pair through here too, but only from their own
            // overlay, so this can count everything, as on EV3.
            bt.pair_count++;
            memcpy(bt.paired_addr, bdaddr, sizeof(bt.paired_addr));
            // A host never sends its name, so ask for it while the link is
            // up, to replace the placeholder.
            const char *name = pbio_bluetooth_classic_link_key_get_name(bdaddr);
            if (name && strcmp(name, BT_HOST_DEFAULT_NAME) == 0) {
                memcpy(bt.host_name_addr, bdaddr, sizeof(bt.host_name_addr));
                bt.host_name_due = true;
                pbio_os_request_poll();
            }
            break;
        }
        case DM_HCI_REMOTE_NAME_COMPLETE: {
            // Words: type, phandle, status, address (4), 8 pointers, then
            // the name, terminated unless it is as long as names get.
            if (size < 7 * 2) {
                break;
            }
            bt.name_busy = false;
            for (uint16_t i = 0; i < 4; i++) {
                addr[i] = bt_get_word(payload, 3 + i);
            }
            bt_addr_from_words(addr, bdaddr);
            char name[PBIO_BLUETOOTH_CLASSIC_NAME_SIZE] = { 0 };
            if (bt_get_word(payload, 2) == 0 && size > 15 * 2) {
                uint16_t length = size - 15 * 2;
                memcpy(name, &payload[15 * 2], length < sizeof(name) - 1 ? length : sizeof(name) - 1);
            }
            DEBUG_PRINT("bt: name status %u, %s\n", bt_get_word(payload, 2), name);
            // Naming a bonding record after its device is right whoever
            // asked, and devices not in the store are ignored.
            if (name[0]) {
                pbio_bluetooth_classic_link_key_set_name(bdaddr, name);
            }
            #if PBDRV_CONFIG_BLUETOOTH_INQUIRY
            inquiry_handle_name(bdaddr, name);
            #endif
            pbio_os_request_poll();
            break;
        }
        case DM_ACL_OPENED_IND:
            bt.acl_count++;
            break;
        case DM_ACL_CLOSED_IND:
            if (bt.acl_count) {
                bt.acl_count--;
            }
            if (memcmp(bdaddr, bt_provisional_addr, sizeof(bdaddr)) == 0) {
                pbio_bluetooth_classic_link_key_unregister(bdaddr);
                memset(bt_provisional_addr, 0, sizeof(bt_provisional_addr));
                DEBUG_PRINT("bt: pairing not completed\n");
            }
            break;
        case DM_SM_AUTHORISE_IND: {
            // Words: type, phandle, address (4), protocol (2), channel (2), incoming.
            const uint16_t reply[] = {
                DM_SM_AUTHORISE_RES, addr[0], addr[1], addr[2], addr[3],
                bt_get_word(payload, 6), bt_get_word(payload, 7),
                bt_get_word(payload, 8), bt_get_word(payload, 9),
                bt_get_word(payload, 10), true,
            };
            bt_reply(BCSP_CHANNEL_DM, reply, PBIO_ARRAY_SIZE(reply));
            break;
        }
        default:
            break;
    }
}

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID || PBDRV_CONFIG_BLUETOOTH_PEER

//
// L2CAP channels, for the gamepad (HID) and the hub network (peers).
//
// Channels are configured both ways with our MTU. Answers to the BC4's
// indications go through the reply queue, so they always come first.
//

typedef enum {
    BT_CHANNEL_FREE,
    /** Connect request due or sent, waiting for the result. */
    BT_CHANNEL_CONNECTING,
    /** Connected, configuring both directions. */
    BT_CHANNEL_CONFIG,
    BT_CHANNEL_OPEN,
    /**
     * Disconnect request due or sent, waiting for the confirmation. Without a
     * cid, it waits for a connect result first.
     */
    BT_CHANNEL_CLOSING,
} bt_channel_state_t;

typedef enum {
    BT_CHANNEL_EVENT_OPENED,
    /** Did not open. */
    BT_CHANNEL_EVENT_FAILED,
    /** Was open, and was closed by the other end or a lost link. */
    BT_CHANNEL_EVENT_CLOSED,
} bt_channel_event_t;

typedef struct {
    bt_channel_state_t state;
    uint16_t cid;
    uint16_t psm;
    /** Remote address, most significant byte first. */
    uint8_t addr[6];
    /** Whether the request of the current state is still to be sent. */
    bool request_due;
    bool configured_in;
    bool configured_out;
    /** Whether the peer slot is taken, also while connecting or after a failure. */
    bool peer_in_use;
    /** Whether this brick paged the other end, which makes it the coordinator. */
    bool is_coordinator;
    /** Result of the connection attempt, ::PBIO_ERROR_AGAIN while pending. */
    pbio_error_t peer_err;
} bt_channel_t;

// Slots: the two HID channels, then one per peer.
#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
#define BT_CHANNEL_HID_CONTROL (0)
#define BT_CHANNEL_HID_INTERRUPT (1)
#define BT_CHANNEL_PEER_FIRST (2)
#else
#define BT_CHANNEL_PEER_FIRST (0)
#endif
#if PBDRV_CONFIG_BLUETOOTH_PEER
#define BT_NUM_CHANNELS (BT_CHANNEL_PEER_FIRST + PBDRV_CONFIG_BLUETOOTH_PEER_MAX_PEERS)
#else
#define BT_NUM_CHANNELS (BT_CHANNEL_PEER_FIRST)
#endif

static bt_channel_t bt_channels[BT_NUM_CHANNELS];

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
static bt_channel_t *hid_channel_accept(uint16_t psm, const uint8_t *addr);
static void hid_channel_event(bt_channel_t *ch, bt_channel_event_t event);
static void hid_handle_data(bt_channel_t *ch, const uint8_t *data, uint16_t size);
#endif
#if PBDRV_CONFIG_BLUETOOTH_PEER
static bt_channel_t *peer_channel_accept(const uint8_t *addr);
static void peer_channel_event(bt_channel_t *ch, bt_channel_event_t event);
static void peer_handle_data(bt_channel_t *ch, const uint8_t *data, uint16_t size);
#endif

static bool bt_channel_is_hid(const bt_channel_t *ch) {
    return ch - bt_channels < BT_CHANNEL_PEER_FIRST;
}

static void bt_channel_event(bt_channel_t *ch, bt_channel_event_t event) {
    DEBUG_PRINT("bt: l2cap psm %04x cid %04x %s\n", ch->psm, ch->cid,
        event == BT_CHANNEL_EVENT_OPENED ? "open" : event == BT_CHANNEL_EVENT_FAILED ? "failed" : "closed");
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    if (bt_channel_is_hid(ch)) {
        hid_channel_event(ch, event);
        return;
    }
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_PEER
    peer_channel_event(ch, event);
    #endif
}

/** Asks for a channel to @p addr on @p psm. The slot must be free. */
static void bt_channel_connect(bt_channel_t *ch, const uint8_t *addr, uint16_t psm) {
    memcpy(ch->addr, addr, sizeof(ch->addr));
    ch->psm = psm;
    ch->cid = 0;
    ch->state = BT_CHANNEL_CONNECTING;
    ch->request_due = true;
    ch->configured_in = false;
    ch->configured_out = false;
    pbio_os_request_poll();
}

/** Closes a channel. Its owner hears nothing more of it. */
static void bt_channel_close(bt_channel_t *ch) {
    switch (ch->state) {
        case BT_CHANNEL_CONNECTING:
            if (ch->request_due) {
                ch->state = BT_CHANNEL_FREE;
                return;
            }
            // Closed once the result is here.
            ch->state = BT_CHANNEL_CLOSING;
            ch->request_due = false;
            break;
        case BT_CHANNEL_CONFIG:
        case BT_CHANNEL_OPEN:
            ch->state = BT_CHANNEL_CLOSING;
            ch->request_due = true;
            pbio_os_request_poll();
            break;
        default:
            break;
    }
}

/** Frees the slot of a channel that is gone, telling the owner if it cares. */
static void bt_channel_gone(bt_channel_t *ch) {
    bt_channel_state_t state = ch->state;
    ch->state = BT_CHANNEL_FREE;
    ch->request_due = false;

    // Confirmations of what was sent on a closed channel may never come.
    bool any = false;
    for (uint32_t i = 0; i < BT_NUM_CHANNELS; i++) {
        any |= bt_channels[i].state == BT_CHANNEL_OPEN;
    }
    if (!any) {
        bt.l2cap_unconfirmed = 0;
    }

    if (state == BT_CHANNEL_OPEN) {
        bt_channel_event(ch, BT_CHANNEL_EVENT_CLOSED);
    } else if (state == BT_CHANNEL_CONNECTING || state == BT_CHANNEL_CONFIG) {
        bt_channel_event(ch, BT_CHANNEL_EVENT_FAILED);
    }
}

/** Forgets all channels, as after a BC4 restart. */
static void bt_channels_reset(void) {
    for (uint32_t i = 0; i < BT_NUM_CHANNELS; i++) {
        bt_channel_gone(&bt_channels[i]);
    }
    bt.l2cap_unconfirmed = 0;
}

static bt_channel_t *bt_channel_for_cid(uint16_t cid) {
    for (uint32_t i = 0; i < BT_NUM_CHANNELS; i++) {
        bt_channel_t *ch = &bt_channels[i];
        if (ch->state != BT_CHANNEL_FREE && ch->cid && ch->cid == cid) {
            return ch;
        }
    }
    return NULL;
}

static void bt_channel_check_open(bt_channel_t *ch) {
    if (ch->state == BT_CHANNEL_CONFIG && ch->configured_in && ch->configured_out) {
        ch->state = BT_CHANNEL_OPEN;
        bt_channel_event(ch, BT_CHANNEL_EVENT_OPENED);
    }
}

/** Finds the slot for an incoming connection, or NULL to refuse it. */
static bt_channel_t *bt_channel_accept(uint16_t psm, const uint8_t *addr) {
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    if (psm == BT_PSM_HID_CONTROL || psm == BT_PSM_HID_INTERRUPT) {
        return hid_channel_accept(psm, addr);
    }
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_PEER
    if (psm == BT_PSM_PEER) {
        return peer_channel_accept(addr);
    }
    #endif
    return NULL;
}

/** Whether an L2CAP data packet can be handed to the BC4 now. */
static bool bt_l2cap_can_write(void) {
    return bcsp_can_send() && !bt.reply_count && bt.l2cap_unconfirmed < BT_L2CAP_UNCONFIRMED_MAX;
}

/** Where the data of the next L2CAP packet goes, BT_L2CAP_MTU long. */
static uint8_t *bt_l2cap_data(void) {
    return bcsp_payload() + 8;
}

/**
 * Sends the L2CAP packet put together in bt_l2cap_data(). Only call when
 * bt_l2cap_can_write() is true.
 */
static void bt_l2cap_send(const bt_channel_t *ch, uint16_t size) {
    // Words: type, cid, length, pointer, then the data. Confirmed by
    // L2CA_DATAWRITE_CFM once sent on.
    const uint16_t msg[] = { L2CA_DATAWRITE_REQ, ch->cid, size, 0 };
    bt_put_words(bcsp_payload(), msg, PBIO_ARRAY_SIZE(msg));
    bcsp_send(BCSP_CHANNEL_L2CAP, sizeof(msg) + size);
    bt.l2cap_unconfirmed++;
}

/** Sends the next request a channel is due, if any. */
static bool bt_channel_send_request(void) {
    for (uint32_t i = 0; i < BT_NUM_CHANNELS; i++) {
        bt_channel_t *ch = &bt_channels[i];
        if (!ch->request_due) {
            continue;
        }
        ch->request_due = false;
        if (ch->state == BT_CHANNEL_CONNECTING) {
            // Words: type, local PSM, address (4), remote PSM, packet type
            // (0: any).
            uint16_t msg[8] = { L2CA_CONNECT_REQ, ch->psm, [6] = ch->psm };
            bt_addr_to_words(ch->addr, &msg[2]);
            bt_send_message(BCSP_CHANNEL_L2CAP, msg, PBIO_ARRAY_SIZE(msg));
        } else if (ch->state == BT_CHANNEL_CONFIG) {
            // Words: type, cid, options (1: MTU), MTU, flowspec (11), flush
            // timeout (none), more.
            const uint16_t msg[17] = { L2CA_CONFIG_REQ, ch->cid, 1, BT_L2CAP_MTU, [15] = 0xffff };
            bt_send_message(BCSP_CHANNEL_L2CAP, msg, PBIO_ARRAY_SIZE(msg));
        } else if (ch->state == BT_CHANNEL_CLOSING) {
            const uint16_t msg[] = { L2CA_DISCONNECT_REQ, ch->cid };
            bt_send_message(BCSP_CHANNEL_L2CAP, msg, PBIO_ARRAY_SIZE(msg));
        } else {
            continue;
        }
        return true;
    }
    return false;
}

static void bt_handle_l2cap(uint16_t type, const uint8_t *payload, uint16_t size) {
    if (size < 8) {
        return;
    }
    bt_channel_t *ch;
    switch (type) {
        case L2CA_CONNECT_IND: {
            // Words: type, phandle, identifier, address (4), cid, psm.
            if (size < 9 * 2) {
                break;
            }
            uint16_t words[4];
            uint8_t addr[6];
            for (uint16_t i = 0; i < 4; i++) {
                words[i] = bt_get_word(payload, 3 + i);
            }
            bt_addr_from_words(words, addr);
            uint16_t cid = bt_get_word(payload, 7);
            uint16_t psm = bt_get_word(payload, 8);
            ch = bt_channel_accept(psm, addr);
            // Words: type, identifier, cid, response.
            const uint16_t reply[] = { L2CA_CONNECT_RSP, bt_get_word(payload, 2), cid, ch ? 0 : L2CA_CONNECT_REFUSED };
            bt_reply(BCSP_CHANNEL_L2CAP, reply, PBIO_ARRAY_SIZE(reply));
            if (!ch) {
                DEBUG_PRINT("bt: l2cap psm %04x refused\n", psm);
                break;
            }
            memcpy(ch->addr, addr, sizeof(addr));
            ch->psm = psm;
            ch->cid = cid;
            ch->state = BT_CHANNEL_CONFIG;
            ch->request_due = true;
            ch->configured_in = false;
            ch->configured_out = false;
            break;
        }
        case L2CA_CONNECT_CFM: {
            // Words: type, phandle, address (4), cid, psm, result.
            if (size < 9 * 2) {
                break;
            }
            uint16_t words[4];
            uint8_t addr[6];
            for (uint16_t i = 0; i < 4; i++) {
                words[i] = bt_get_word(payload, 2 + i);
            }
            bt_addr_from_words(words, addr);
            uint16_t psm = bt_get_word(payload, 7);
            uint16_t result = bt_get_word(payload, 8);
            for (ch = bt_channels; ch < &bt_channels[BT_NUM_CHANNELS]; ch++) {
                if ((ch->state == BT_CHANNEL_CONNECTING || (ch->state == BT_CHANNEL_CLOSING && !ch->cid)) &&
                    !ch->request_due && ch->psm == psm && memcmp(ch->addr, addr, sizeof(addr)) == 0) {
                    break;
                }
            }
            if (ch == &bt_channels[BT_NUM_CHANNELS] || result == L2CA_CONNECT_PENDING) {
                break;
            }
            if (result) {
                DEBUG_PRINT("bt: l2cap connect result %04x\n", result);
                bt_channel_gone(ch);
                break;
            }
            ch->cid = bt_get_word(payload, 6);
            // Configure it, or close it if that was asked for meanwhile.
            ch->request_due = true;
            if (ch->state == BT_CHANNEL_CONNECTING) {
                ch->state = BT_CHANNEL_CONFIG;
            }
            break;
        }
        case L2CA_CONFIG_IND: {
            // Words: type, phandle, cid, identifier, options size, options,
            // more. The options are about what the other end receives, so
            // they are fine as they are.
            ch = bt_channel_for_cid(bt_get_word(payload, 2));
            const uint16_t reply[] = { L2CA_CONFIG_RSP, bt_get_word(payload, 2), bt_get_word(payload, 3), 0, 0, 0, 0 };
            bt_reply(BCSP_CHANNEL_L2CAP, reply, PBIO_ARRAY_SIZE(reply));
            if (ch) {
                ch->configured_in = true;
                bt_channel_check_open(ch);
            }
            break;
        }
        case L2CA_CONFIG_CFM:
            // Words: type, phandle, cid, result, options, MTU, ...
            ch = bt_channel_for_cid(bt_get_word(payload, 2));
            if (!ch || ch->state != BT_CHANNEL_CONFIG) {
                break;
            }
            if (bt_get_word(payload, 3)) {
                DEBUG_PRINT("bt: l2cap config result %04x\n", bt_get_word(payload, 3));
                bt_channel_close(ch);
                bt_channel_event(ch, BT_CHANNEL_EVENT_FAILED);
                break;
            }
            ch->configured_out = true;
            bt_channel_check_open(ch);
            break;
        case L2CA_DATAREAD_IND: {
            // Words: type, phandle, cid, psm, length, two pointers of the
            // BC4's own, then the data.
            if (size < 14) {
                break;
            }
            uint16_t length = bt_get_word(payload, 4);
            ch = bt_channel_for_cid(bt_get_word(payload, 2));
            if (!ch || ch->state != BT_CHANNEL_OPEN || size - 14 < length) {
                break;
            }
            #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
            if (bt_channel_is_hid(ch)) {
                hid_handle_data(ch, &payload[14], length);
                break;
            }
            #endif
            #if PBDRV_CONFIG_BLUETOOTH_PEER
            peer_handle_data(ch, &payload[14], length);
            #endif
            break;
        }
        case L2CA_DATAWRITE_CFM:
            // Words: type, phandle, cid, length, result.
            if (bt.l2cap_unconfirmed) {
                bt.l2cap_unconfirmed--;
            }
            break;
        case L2CA_DISCONNECT_IND: {
            // Words: type, phandle, identifier, cid.
            const uint16_t reply[] = { L2CA_DISCONNECT_RSP, bt_get_word(payload, 2), bt_get_word(payload, 3) };
            bt_reply(BCSP_CHANNEL_L2CAP, reply, PBIO_ARRAY_SIZE(reply));
            ch = bt_channel_for_cid(bt_get_word(payload, 3));
            if (ch) {
                bt_channel_gone(ch);
            }
            break;
        }
        case L2CA_DISCONNECT_CFM:
            // Words: type, phandle, cid, result.
            ch = bt_channel_for_cid(bt_get_word(payload, 2));
            if (ch && ch->state == BT_CHANNEL_CLOSING) {
                bt_channel_gone(ch);
            }
            break;
        default:
            break;
    }
}

#endif // PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID || PBDRV_CONFIG_BLUETOOTH_PEER

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID

//
// HID host, for gamepads.
//
// Pairing: an SDP query for the HID descriptor (which also opens the link),
// authentication with the gamepad PIN, then the control and interrupt
// channels. A bonded gamepad then connects by itself, and we query its
// descriptor once it has.
//
// Gamepads send far more reports than the BC4 can pass on (~240 per second)
// and queue the rest, which shows as lag. So once connected, the link goes
// into sniff mode with this brick as master: the gamepad then sends one
// report per sniff interval and drops the ones it cannot send. Gamepads
// refuse sniff from a slave, so the role is switched first if the gamepad
// paged us.
//

/** PIN for legacy pairing with a gamepad, as on EV3. */
#define HID_PIN_CODE "0000"

/** How long pairing may take, including paging the gamepad. */
#define HID_PAIR_TIMEOUT_MS (30000)

/** Most attribute bytes the SDP query may return. */
#define HID_SDP_MAX_BYTES (BCSP_PAYLOAD_MAX - 64)

/**
 * Sniff interval in slots of 0.625 ms: one report per interval, so 10 gives
 * 160 reports per second, leaving the BC4 some room.
 */
#define HID_SNIFF_INTERVAL (10)

/**
 * Sniff attempt and timeout, in slots. A timeout of 1 keeps the gamepad to
 * one report per interval.
 */
#define HID_SNIFF_ATTEMPT (2)
#define HID_SNIFF_TIMEOUT (1)

typedef enum {
    HID_IDLE,
    HID_PAIRING,
    HID_CONNECTING,
    HID_CONNECTED,
} hid_state_t;

/** Steps that put the link into sniff mode, as master. */
typedef enum {
    HID_LINK_ACTIVE,
    HID_LINK_ROLE_DUE,
    HID_LINK_ROLE_WAIT,
    HID_LINK_SWITCH_DUE,
    HID_LINK_SWITCH_WAIT,
    HID_LINK_SNIFF_DUE,
    HID_LINK_SNIFF_WAIT,
    /** Sniff mode, or given up on it. */
    HID_LINK_DONE,
} hid_link_step_t;

static struct {
    hid_state_t state;
    hid_link_step_t link_step;
    /** Address of the gamepad, most significant byte first. */
    uint8_t addr[6];
    /** Result of the last pairing session. */
    pbio_error_t pair_err;
    /** Ends the pairing session if it takes too long. */
    pbio_os_timer_t pair_timer;
    /** Whether the SDP query is due, and whether it is running. */
    bool query_due;
    bool query_busy;
    /** Whether authentication is due while pairing. */
    bool authenticate_due;
    /** Whether the link is to be dropped. */
    bool close_link_due;
    uint8_t close_link_addr[6];
    /** Whether the descriptor has been looked at. Reports wait for it. */
    bool descriptor_known;
    /** Whether the gamepad declares report IDs. Only valid once known. */
    bool report_id_declared;
    /** Most recent input report of each ID, as on EV3. */
    struct {
        uint8_t data[PBDRV_BLUETOOTH_HID_MAX_REPORT_SIZE];
        uint8_t size;
        uint8_t id;
    } reports[PBDRV_BLUETOOTH_HID_NUM_REPORTS];
} hid;

#define hid_control (&bt_channels[BT_CHANNEL_HID_CONTROL])
#define hid_interrupt (&bt_channels[BT_CHANNEL_HID_INTERRUPT])

static const char *hid_pin_code(const uint8_t *bdaddr) {
    if (hid.state == HID_PAIRING && memcmp(bdaddr, hid.addr, sizeof(hid.addr)) == 0) {
        return HID_PIN_CODE;
    }
    return BT_PIN_CODE;
}

static void hid_report_store(const uint8_t *data, uint8_t size) {
    if (!hid.descriptor_known) {
        return;
    }
    if (size > PBDRV_BLUETOOTH_HID_MAX_REPORT_SIZE) {
        size = PBDRV_BLUETOOTH_HID_MAX_REPORT_SIZE;
    }
    uint8_t id = hid.report_id_declared ? data[0] : 0;
    uint32_t slot = PBDRV_BLUETOOTH_HID_NUM_REPORTS;
    for (uint32_t i = 0; i < PBDRV_BLUETOOTH_HID_NUM_REPORTS; i++) {
        if (hid.reports[i].size == 0) {
            if (slot == PBDRV_BLUETOOTH_HID_NUM_REPORTS) {
                slot = i;
            }
            continue;
        }
        if (hid.reports[i].id == id) {
            slot = i;
            break;
        }
    }
    if (slot == PBDRV_BLUETOOTH_HID_NUM_REPORTS) {
        return;
    }
    memcpy(hid.reports[slot].data, data, size);
    hid.reports[slot].size = size;
    hid.reports[slot].id = id;
}

static void hid_handle_data(bt_channel_t *ch, const uint8_t *data, uint16_t size) {
    // Input reports come on the interrupt channel as DATA | INPUT (0xa1),
    // then the report.
    if (ch == hid_interrupt && hid.state == HID_CONNECTED && size > 1 && data[0] == 0xa1) {
        hid_report_store(&data[1], size - 1);
    }
}

/** Closes both channels and the link. */
static void hid_close(void) {
    if (hid.state == HID_CONNECTED) {
        pbsys_host_connection_changed();
    }
    bt_channel_close(hid_control);
    bt_channel_close(hid_interrupt);
    // Closing the channels leaves the link up for a while, and the gamepad
    // awake. Drop it, so that it sees the disconnect at once.
    memcpy(hid.close_link_addr, hid.addr, sizeof(hid.addr));
    hid.close_link_due = true;
    hid.state = HID_IDLE;
    hid.link_step = HID_LINK_ACTIVE;
    hid.query_due = false;
    hid.authenticate_due = false;
    pbio_os_request_poll();
}

static void hid_pair_end(pbio_error_t err) {
    DEBUG_PRINT("bt: hid pairing ended, %d\n", err);
    hid.pair_err = err;
    if (err != PBIO_SUCCESS) {
        hid_close();
        pbio_bluetooth_classic_link_key_unregister(hid.addr);
    }
}

static bt_channel_t *hid_channel_accept(uint16_t psm, const uint8_t *addr) {
    bt_channel_t *ch = psm == BT_PSM_HID_CONTROL ? hid_control : hid_interrupt;
    if (ch->state != BT_CHANNEL_FREE || hid.state == HID_CONNECTED || bt.shutdown_requested) {
        return NULL;
    }
    if (hid.state == HID_IDLE) {
        // A bonded gamepad reconnecting. Unknown devices are not let in.
        if (psm != BT_PSM_HID_CONTROL || !pbio_bluetooth_classic_link_key_get_name(addr)) {
            return NULL;
        }
        memcpy(hid.addr, addr, sizeof(hid.addr));
        hid.state = HID_CONNECTING;
        hid.descriptor_known = false;
        return ch;
    }
    // While pairing, the gamepad may open the channels itself.
    return memcmp(addr, hid.addr, sizeof(hid.addr)) == 0 ? ch : NULL;
}

static void hid_channel_event(bt_channel_t *ch, bt_channel_event_t event) {
    if (hid.state == HID_IDLE) {
        return;
    }
    if (event != BT_CHANNEL_EVENT_OPENED) {
        // While pairing, the gamepad may still connect by itself, so the
        // session ends on success or time out.
        if (hid.state != HID_PAIRING) {
            hid_close();
        }
        return;
    }
    if (hid_control->state == BT_CHANNEL_OPEN && hid_interrupt->state == BT_CHANNEL_OPEN) {
        for (uint32_t i = 0; i < PBDRV_BLUETOOTH_HID_NUM_REPORTS; i++) {
            hid.reports[i].size = 0;
        }
        if (hid.state == HID_PAIRING) {
            hid_pair_end(PBIO_SUCCESS);
        }
        hid.state = HID_CONNECTED;
        hid.link_step = HID_LINK_ROLE_DUE;
        pbsys_host_connection_changed();
        if (!hid.descriptor_known && !hid.query_busy) {
            hid.query_due = true;
        }
        return;
    }
    // After the control channel comes the interrupt channel.
    if (ch == hid_control && hid.state == HID_PAIRING && hid_interrupt->state == BT_CHANNEL_FREE) {
        bt_channel_connect(hid_interrupt, hid.addr, BT_PSM_HID_INTERRUPT);
    }
}

/**
 * Whether the HID descriptor in an SDP answer declares report IDs. The
 * answer is searched for attribute 0x0206 (HIDDescriptorList), a list of
 * (type, descriptor) pairs. Anything unexpected counts as yes, as on EV3.
 */
static bool hid_sdp_report_id_declared(const uint8_t *data, uint16_t size) {
    static const uint8_t attribute[] = { 0x09, 0x02, 0x06 };
    uint16_t i = 0;
    while (i + sizeof(attribute) <= size && memcmp(&data[i], attribute, sizeof(attribute)) != 0) {
        i++;
    }
    i += sizeof(attribute);
    // Sequence headers, then the type (0x22: report descriptor).
    while (i + 1 < size && (data[i] == 0x35 || data[i] == 0x36)) {
        i += data[i] == 0x35 ? 2 : 3;
    }
    if (i + 4 > size || data[i] != 0x08) {
        return true;
    }
    i += 2;
    uint16_t length;
    if (data[i] == 0x25) {
        length = data[i + 1];
        i += 2;
    } else if (data[i] == 0x26) {
        length = data[i + 1] << 8 | data[i + 2];
        i += 3;
    } else {
        return true;
    }
    uint16_t end = i + length < size ? i + length : size;
    DEBUG_PRINT("bt: hid descriptor %u bytes, %u here\n", length, end - i);
    // Short items: a prefix with the size in its low two bits. Report ID is 0x85.
    while (i < end) {
        uint8_t prefix = data[i];
        if (prefix == 0xfe) {
            // Long item: size in the next byte.
            i += i + 1 < end ? 3 + data[i + 1] : 1;
            continue;
        }
        if ((prefix & 0xfc) == 0x84) {
            return true;
        }
        i += 1 + ((prefix & 3) == 3 ? 4 : (prefix & 3));
    }
    return false;
}

static void hid_handle_dm(uint16_t type, const uint8_t *payload, uint16_t size) {
    uint16_t words[4];
    uint8_t addr[6];

    // Link policy results. Words: type, phandle, then for mode change:
    // status, address (4), mode, interval; for role discovery and switch:
    // address (4), status, role.
    if (type == DM_HCI_MODE_CHANGE_EVENT || type == DM_HCI_ROLE_DISCOVERY_COMPLETE || type == DM_HCI_SWITCH_ROLE_COMPLETE) {
        bool mode = type == DM_HCI_MODE_CHANGE_EVENT;
        if (size < (mode ? 9 : 8) * 2 || hid.state != HID_CONNECTED) {
            return;
        }
        for (uint16_t i = 0; i < 4; i++) {
            words[i] = bt_get_word(payload, (mode ? 3 : 2) + i);
        }
        bt_addr_from_words(words, addr);
        if (memcmp(addr, hid.addr, sizeof(addr)) != 0) {
            return;
        }
        // Mode or role.
        uint16_t status = bt_get_word(payload, mode ? 2 : 6);
        uint16_t value = bt_get_word(payload, 7);
        DEBUG_PRINT("bt: hid link %04x status %02x, %u\n", type, status, value);
        if (mode) {
            if (hid.link_step == HID_LINK_SNIFF_WAIT) {
                hid.link_step = HID_LINK_DONE;
            }
            return;
        }
        // Role 0 is master. Sniff only works as master, so stay active
        // if that fails.
        bool master = status == 0 && value == 0;
        if (type == DM_HCI_ROLE_DISCOVERY_COMPLETE && hid.link_step == HID_LINK_ROLE_WAIT) {
            hid.link_step = master ? HID_LINK_SNIFF_DUE : status == 0 ? HID_LINK_SWITCH_DUE : HID_LINK_DONE;
        } else if (type == DM_HCI_SWITCH_ROLE_COMPLETE && hid.link_step == HID_LINK_SWITCH_WAIT) {
            hid.link_step = master ? HID_LINK_SNIFF_DUE : HID_LINK_DONE;
        }
        pbio_os_request_poll();
        return;
    }

    // Words: type, phandle, address (4), success.
    if (type != DM_SM_AUTHENTICATE_CFM || size < 7 * 2 || hid.state != HID_PAIRING) {
        return;
    }
    for (uint16_t i = 0; i < 4; i++) {
        words[i] = bt_get_word(payload, 2 + i);
    }
    bt_addr_from_words(words, addr);
    if (memcmp(addr, hid.addr, sizeof(addr)) != 0) {
        return;
    }
    DEBUG_PRINT("bt: hid authentication %s\n", bt_get_word(payload, 6) ? "done" : "failed");
    if (!bt_get_word(payload, 6)) {
        hid_pair_end(PBIO_ERROR_FAILED);
    } else if (hid_control->state == BT_CHANNEL_FREE) {
        bt_channel_connect(hid_control, hid.addr, BT_PSM_HID_CONTROL);
    }
}

static void hid_handle_sdp(uint16_t type, const uint8_t *payload, uint16_t size) {
    if (type != SDC_SERVICE_SEARCH_ATTRIBUTE_CFM || !hid.query_busy) {
        return;
    }
    hid.query_busy = false;
    hid.descriptor_known = true;
    hid.report_id_declared = hid_sdp_report_id_declared(payload, size);
    DEBUG_PRINT("bt: hid report IDs %s\n", hid.report_id_declared ? "declared" : "not declared");
    if (hid.state == HID_PAIRING) {
        hid.authenticate_due = true;
    }
}

/** Sends what the link to the gamepad needs next to go into sniff mode, if anything. */
static bool hid_link_send_next(void) {
    // Wait for the descriptor, which comes quicker on an active link.
    if (hid.state != HID_CONNECTED || hid.query_due || hid.query_busy) {
        return false;
    }
    // Words: type, length in words, then the handle (filled in by the DM)
    // and the address, except for switch role.
    uint16_t msg[11] = { 0, 0, 0 };
    switch (hid.link_step) {
        case HID_LINK_ROLE_DUE:
            msg[0] = DM_HCI_ROLE_DISCOVERY;
            msg[1] = 7;
            bt_addr_to_words(hid.addr, &msg[3]);
            hid.link_step = HID_LINK_ROLE_WAIT;
            break;
        case HID_LINK_SWITCH_DUE:
            // Words: type, length, address (4), role (0: master).
            msg[0] = DM_HCI_SWITCH_ROLE;
            msg[1] = 7;
            bt_addr_to_words(hid.addr, &msg[2]);
            msg[6] = 0;
            hid.link_step = HID_LINK_SWITCH_WAIT;
            break;
        case HID_LINK_SNIFF_DUE:
            // Then: max interval, min interval, attempt, timeout.
            msg[0] = DM_HCI_SNIFF_MODE;
            msg[1] = 11;
            bt_addr_to_words(hid.addr, &msg[3]);
            msg[7] = HID_SNIFF_INTERVAL;
            msg[8] = HID_SNIFF_INTERVAL;
            msg[9] = HID_SNIFF_ATTEMPT;
            msg[10] = HID_SNIFF_TIMEOUT;
            hid.link_step = HID_LINK_SNIFF_WAIT;
            break;
        default:
            return false;
    }
    bt_send_message(BCSP_CHANNEL_DM, msg, msg[1]);
    return true;
}

/** Sends what the gamepad connection needs next, if anything. */
static bool hid_send_next(void) {
    uint16_t msg[11];

    if (hid.close_link_due) {
        hid.close_link_due = false;
        msg[0] = DM_ACL_CLOSE_REQ;
        bt_addr_to_words(hid.close_link_addr, &msg[1]);
        bt_send_message(BCSP_CHANNEL_DM, msg, 5);
        return true;
    }

    if (hid.state == HID_PAIRING && pbio_os_timer_is_expired(&hid.pair_timer)) {
        hid_pair_end(PBIO_ERROR_TIMEDOUT);
    }

    if (hid.query_due) {
        // Words: type, phandle, address (4), search pattern size, pointer,
        // attribute list size, pointer, most attribute bytes. The search
        // pattern and the attribute list follow, in that order.
        static const uint8_t query[] = {
            // Search pattern: the HID service class.
            0x35, 0x03, 0x19, 0x11, 0x24,
            // Attributes: the HID descriptor list.
            0x35, 0x03, 0x09, 0x02, 0x06,
        };
        hid.query_due = false;
        hid.query_busy = true;
        msg[0] = SDC_SERVICE_SEARCH_ATTRIBUTE_REQ;
        msg[1] = BT_PHANDLE;
        bt_addr_to_words(hid.addr, &msg[2]);
        msg[6] = 5;
        msg[7] = 0;
        msg[8] = 5;
        msg[9] = 0;
        msg[10] = HID_SDP_MAX_BYTES;
        bt_send_message_with_data(BCSP_CHANNEL_SDP, msg, 11, query, sizeof(query));
        return true;
    }

    if (hid.authenticate_due) {
        hid.authenticate_due = false;
        msg[0] = DM_SM_AUTHENTICATE_REQ;
        bt_addr_to_words(hid.addr, &msg[1]);
        bt_send_message(BCSP_CHANNEL_DM, msg, 5);
        return true;
    }

    return hid_link_send_next();
}

/** Forgets the gamepad connection, as after a BC4 restart. */
static void hid_reset(void) {
    if (hid.state == HID_PAIRING) {
        hid_pair_end(PBIO_ERROR_FAILED);
    }
    hid.state = HID_IDLE;
    hid.link_step = HID_LINK_ACTIVE;
    hid.query_due = false;
    hid.query_busy = false;
    hid.authenticate_due = false;
    hid.close_link_due = false;
}

pbio_error_t pbdrv_bluetooth_classic_hid_pair(const uint8_t *bdaddr, const char *name) {
    if (!bt.ready || bt.shutdown_requested) {
        return PBIO_ERROR_INVALID_OP;
    }
    if (hid.state != HID_IDLE) {
        return PBIO_ERROR_BUSY;
    }
    DEBUG_PRINT("bt: pairing %s\n", name);
    // Provisional bonding record, which gets the key once it comes.
    pbio_bluetooth_classic_link_key_register(bdaddr, name);
    memcpy(hid.addr, bdaddr, sizeof(hid.addr));
    hid.state = HID_PAIRING;
    hid.pair_err = PBIO_ERROR_AGAIN;
    hid.descriptor_known = false;
    hid.query_due = true;
    pbio_os_timer_set(&hid.pair_timer, HID_PAIR_TIMEOUT_MS);
    pbio_os_request_poll();
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_classic_hid_pair_status(void) {
    return hid.state == HID_PAIRING ? PBIO_ERROR_AGAIN : hid.pair_err;
}

void pbdrv_bluetooth_classic_hid_pair_cancel(void) {
    if (hid.state == HID_PAIRING) {
        hid_pair_end(PBIO_ERROR_CANCELED);
    }
}

bool pbdrv_bluetooth_classic_hid_is_connected(void) {
    return hid.state == HID_CONNECTED;
}

uint32_t pbdrv_bluetooth_classic_hid_get_report(uint8_t report_id, uint8_t *data, uint32_t size) {
    if (hid.state != HID_CONNECTED) {
        return 0;
    }
    for (uint32_t i = 0; i < PBDRV_BLUETOOTH_HID_NUM_REPORTS; i++) {
        if (hid.reports[i].size == 0 || hid.reports[i].id != report_id) {
            continue;
        }
        if (size > hid.reports[i].size) {
            size = hid.reports[i].size;
        }
        memcpy(data, hid.reports[i].data, size);
        return size;
    }
    return 0;
}

bool pbdrv_bluetooth_classic_hid_get_report_id(uint32_t index, uint8_t *report_id) {
    if (hid.state != HID_CONNECTED || index >= PBDRV_BLUETOOTH_HID_NUM_REPORTS || hid.reports[index].size == 0) {
        return false;
    }
    *report_id = hid.reports[index].id;
    return true;
}

const char *pbdrv_bluetooth_classic_hid_get_connected_name(void) {
    return hid.state == HID_CONNECTED ? pbio_bluetooth_classic_link_key_get_name(hid.addr) : NULL;
}

void pbdrv_bluetooth_classic_hid_disconnect(void) {
    if (hid.state == HID_PAIRING) {
        hid_pair_end(PBIO_ERROR_CANCELED);
    } else if (hid.state != HID_IDLE) {
        hid_close();
    }
}

#endif // PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID

#if PBDRV_CONFIG_BLUETOOTH_PEER

//
// Hub network: brick-to-brick messages over one L2CAP channel per peer, as on
// EV3 (bluetooth_btstack.c), which this follows closely. The topology is a
// star: only the coordinator pages, and it relays between its peers. Nothing
// is paired.
//

/** Message type byte. Only data is defined. */
#define PEER_MESSAGE_TYPE_DATA (0)

const uint8_t pbdrv_bluetooth_peer_address_all[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

#define peer_channels (&bt_channels[BT_CHANNEL_PEER_FIRST])
#define PEER_NUM_CHANNELS (PBDRV_CONFIG_BLUETOOTH_PEER_MAX_PEERS)

static pbdrv_bluetooth_peer_receive_callback_t peer_receive_callback;

/** The message this brick sends on its own behalf, borrowed from the caller. */
static struct {
    const uint8_t *data;
    uint32_t size;
    uint8_t address[6];
    /** Channels still to be given this message, by slot index. */
    uint32_t pending;
} peer_tx_local;

// Relay ring, as on EV3: each message has the channels still to be given it,
// the payload size, and the address for the header in front of it.
#define PEER_RELAY_PENDING_OFFSET (0)
#define PEER_RELAY_SIZE_OFFSET (4)
#define PEER_RELAY_ADDRESS_OFFSET (6)

static uint8_t *peer_relay_buf;
static uint32_t peer_relay_capacity;
static uint32_t peer_relay_tail;
static uint32_t peer_relay_used;
static uint32_t peer_relay_count;

static bool peer_is_connected(const bt_channel_t *ch) {
    return ch->peer_in_use && ch->state == BT_CHANNEL_OPEN;
}

static bool peer_address_is_all(const uint8_t *address) {
    return memcmp(address, pbdrv_bluetooth_peer_address_all, 6) == 0;
}

static bt_channel_t *peer_channel_for_address(const uint8_t *address) {
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        bt_channel_t *ch = &peer_channels[i];
        if (ch->peer_in_use && memcmp(ch->addr, address, 6) == 0) {
            return ch;
        }
    }
    return NULL;
}

/** Takes a free slot, else recycles one left behind by a failed attempt. */
static bt_channel_t *peer_channel_alloc(void) {
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        bt_channel_t *ch = &peer_channels[i];
        if (!ch->peer_in_use && ch->state == BT_CHANNEL_FREE) {
            return ch;
        }
    }
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        bt_channel_t *ch = &peer_channels[i];
        if (ch->state == BT_CHANNEL_FREE && ch->peer_err != PBIO_ERROR_AGAIN) {
            return ch;
        }
    }
    return NULL;
}

/** The channel to the coordinator, if this brick is a peer. */
static bt_channel_t *peer_channel_uplink(void) {
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        bt_channel_t *ch = &peer_channels[i];
        if (peer_is_connected(ch) && !ch->is_coordinator) {
            return ch;
        }
    }
    return NULL;
}

/**
 * Whether this brick has a live channel that it initiated (@p initiated) or
 * accepted. The star has one coordinator, so either kind rules out the other.
 */
static bool peer_has_live_channel(bool initiated) {
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        bt_channel_t *ch = &peer_channels[i];
        if (ch->peer_in_use && ch->is_coordinator == initiated &&
            (peer_is_connected(ch) || ch->peer_err == PBIO_ERROR_AGAIN)) {
            return true;
        }
    }
    return false;
}

static void peer_relay_read(uint32_t pos, void *dest, uint32_t size) {
    uint8_t *bytes = dest;
    for (uint32_t i = 0; i < size; i++) {
        bytes[i] = peer_relay_buf[(pos + i) % peer_relay_capacity];
    }
}

static void peer_relay_write(uint32_t pos, const void *src, uint32_t size) {
    const uint8_t *bytes = src;
    for (uint32_t i = 0; i < size; i++) {
        peer_relay_buf[(pos + i) % peer_relay_capacity] = bytes[i];
    }
}

static void peer_relay_discard(void) {
    uint8_t size[2];
    peer_relay_read(peer_relay_tail + PEER_RELAY_SIZE_OFFSET, size, sizeof(size));
    uint32_t total = PBDRV_BLUETOOTH_PEER_RELAY_OVERHEAD + pbio_get_uint16_le(size);
    peer_relay_tail = (peer_relay_tail + total) % peer_relay_capacity;
    peer_relay_used -= total;
    peer_relay_count--;
}

static void peer_relay_set_head_pending(uint32_t pending) {
    uint8_t field[4];
    pbio_set_uint32_le(field, pending);
    peer_relay_write(peer_relay_tail + PEER_RELAY_PENDING_OFFSET, field, sizeof(field));
}

/**
 * Takes the next connected destination out of @p pending, forgetting any
 * channel that closed meanwhile. Its bit is left set.
 */
static bt_channel_t *peer_tx_next_channel(uint32_t *pending) {
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        if (!(*pending & (1U << i))) {
            continue;
        }
        if (peer_is_connected(&peer_channels[i])) {
            return &peer_channels[i];
        }
        *pending &= ~(1U << i);
    }
    return NULL;
}

/** Puts the message header in the next L2CAP packet, returning where the payload goes. */
static uint8_t *peer_tx_begin(const uint8_t *address) {
    uint8_t *out = bt_l2cap_data();
    out[0] = PEER_MESSAGE_TYPE_DATA;
    memcpy(&out[1], address, 6);
    return &out[PBDRV_BLUETOOTH_PEER_HEADER_SIZE];
}

/**
 * Hands the BC4 at most one message. Relay traffic goes first, as on EV3.
 *
 * @return  Whether a message was sent.
 */
static bool peer_tx_pump(void) {

    while (peer_relay_count) {
        uint8_t header[PBDRV_BLUETOOTH_PEER_RELAY_OVERHEAD];
        peer_relay_read(peer_relay_tail, header, sizeof(header));
        uint32_t pending = pbio_get_uint32_le(&header[PEER_RELAY_PENDING_OFFSET]);
        uint32_t size = pbio_get_uint16_le(&header[PEER_RELAY_SIZE_OFFSET]);

        bt_channel_t *ch = peer_tx_next_channel(&pending);
        if (!ch) {
            peer_relay_discard();
            continue;
        }
        if (!bt_l2cap_can_write()) {
            peer_relay_set_head_pending(pending);
            return false;
        }
        peer_relay_read(peer_relay_tail + sizeof(header), peer_tx_begin(&header[PEER_RELAY_ADDRESS_OFFSET]), size);
        bt_l2cap_send(ch, PBDRV_BLUETOOTH_PEER_HEADER_SIZE + size);
        peer_relay_set_head_pending(pending & ~(1U << (uint32_t)(ch - peer_channels)));
        return true;
    }

    while (peer_tx_local.pending) {
        bt_channel_t *ch = peer_tx_next_channel(&peer_tx_local.pending);
        if (!ch) {
            break;
        }
        if (!bt_l2cap_can_write()) {
            return false;
        }
        memcpy(peer_tx_begin(peer_tx_local.address), peer_tx_local.data, peer_tx_local.size);
        bt_l2cap_send(ch, PBDRV_BLUETOOTH_PEER_HEADER_SIZE + peer_tx_local.size);
        peer_tx_local.pending &= ~(1U << (uint32_t)(ch - peer_channels));
        return true;
    }

    // Stop borrowing the caller's payload once it is not needed.
    peer_tx_local.data = NULL;
    return false;
}

/** Queues a message to be passed on to every channel in @p mask, best effort. */
static void peer_relay_enqueue(uint32_t mask, const uint8_t *address, const uint8_t *data, uint32_t size) {
    uint32_t total = PBDRV_BLUETOOTH_PEER_RELAY_OVERHEAD + size;
    if (!mask || !peer_relay_buf || total > peer_relay_capacity) {
        return;
    }
    uint8_t header[PBDRV_BLUETOOTH_PEER_RELAY_OVERHEAD];
    pbio_set_uint32_le(&header[PEER_RELAY_PENDING_OFFSET], mask);
    pbio_set_uint16_le(&header[PEER_RELAY_SIZE_OFFSET], size);
    memcpy(&header[PEER_RELAY_ADDRESS_OFFSET], address, 6);
    // Dropping the oldest keeps the freshest data moving.
    while (peer_relay_used + total > peer_relay_capacity) {
        peer_relay_discard();
    }
    uint32_t pos = (peer_relay_tail + peer_relay_used) % peer_relay_capacity;
    peer_relay_write(pos, header, sizeof(header));
    peer_relay_write(pos + sizeof(header), data, size);
    peer_relay_used += total;
    peer_relay_count++;
    pbio_os_request_poll();
}

static void peer_handle_data(bt_channel_t *ch, const uint8_t *packet, uint16_t size) {

    if (size < PBDRV_BLUETOOTH_PEER_HEADER_SIZE || packet[0] != PEER_MESSAGE_TYPE_DATA) {
        return;
    }
    const uint8_t *address = &packet[1];
    const uint8_t *payload = &packet[PBDRV_BLUETOOTH_PEER_HEADER_SIZE];
    uint32_t payload_size = size - PBDRV_BLUETOOTH_PEER_HEADER_SIZE;

    if (!ch->is_coordinator) {
        // On a peer, the header carries the original sender.
        if (peer_receive_callback) {
            peer_receive_callback(address, payload, payload_size);
        }
        return;
    }

    // On the coordinator, the header carries the destination, and the sender
    // is whoever owns this channel.
    bool to_all = peer_address_is_all(address);
    bool to_us = memcmp(address, bt.local_addr, 6) == 0;
    if (peer_receive_callback && (to_all || to_us)) {
        peer_receive_callback(ch->addr, payload, payload_size);
    }
    if (to_us) {
        return;
    }
    // Relay, never back to the sender.
    uint32_t mask = 0;
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        bt_channel_t *out = &peer_channels[i];
        if (out != ch && peer_is_connected(out) && (to_all || memcmp(out->addr, address, 6) == 0)) {
            mask |= 1U << i;
        }
    }
    peer_relay_enqueue(mask, ch->addr, payload, payload_size);
}

static bt_channel_t *peer_channel_accept(const uint8_t *addr) {
    // A brick already in a star, as coordinator or as peer, takes no other
    // link, so two bricks paging each other both fail.
    if (peer_has_live_channel(true) || peer_has_live_channel(false)) {
        return NULL;
    }
    bt_channel_t *ch = peer_channel_for_address(addr);
    if (!ch) {
        ch = peer_channel_alloc();
    }
    if (!ch || ch->state != BT_CHANNEL_FREE) {
        return NULL;
    }
    ch->peer_in_use = true;
    ch->is_coordinator = false;
    ch->peer_err = PBIO_ERROR_AGAIN;
    return ch;
}

static void peer_channel_event(bt_channel_t *ch, bt_channel_event_t event) {
    if (!ch->peer_in_use) {
        return;
    }
    if (event == BT_CHANNEL_EVENT_OPENED) {
        ch->peer_err = PBIO_SUCCESS;
    } else if (event == BT_CHANNEL_EVENT_FAILED) {
        ch->peer_err = PBIO_ERROR_FAILED;
    } else {
        // Released entirely: nothing reconnects by itself.
        ch->peer_in_use = false;
    }
    pbio_os_request_poll();
}

static void peer_reset(void) {
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        peer_channels[i].peer_in_use = false;
    }
    peer_tx_local.pending = 0;
    peer_tx_local.data = NULL;
}

bool pbdrv_bluetooth_peer_is_ready(void) {
    return bt.ready;
}

void pbdrv_bluetooth_peer_set_receive_callback(pbdrv_bluetooth_peer_receive_callback_t callback) {
    peer_receive_callback = callback;
}

void pbdrv_bluetooth_peer_set_relay_buffer(uint8_t *buffer, uint32_t size) {
    peer_relay_buf = size >= PBDRV_BLUETOOTH_PEER_RELAY_MIN_SIZE ? buffer : NULL;
    peer_relay_capacity = peer_relay_buf ? size : 0;
    peer_relay_tail = 0;
    peer_relay_used = 0;
    peer_relay_count = 0;
}

pbio_error_t pbdrv_bluetooth_peer_connect(const uint8_t *address) {
    if (!pbdrv_bluetooth_peer_is_ready() || peer_has_live_channel(false)) {
        return PBIO_ERROR_INVALID_OP;
    }
    bt_channel_t *ch = peer_channel_for_address(address);
    if (ch && (peer_is_connected(ch) || ch->peer_err == PBIO_ERROR_AGAIN)) {
        return PBIO_SUCCESS;
    }
    if (!ch) {
        ch = peer_channel_alloc();
    }
    if (!ch) {
        return PBIO_ERROR_NO_DEV;
    }
    DEBUG_PRINT("bt: connecting to peer %02x:%02x:%02x:%02x:%02x:%02x\n",
        address[0], address[1], address[2], address[3], address[4], address[5]);
    ch->peer_in_use = true;
    ch->is_coordinator = true;
    ch->peer_err = PBIO_ERROR_AGAIN;
    bt_channel_connect(ch, address, BT_PSM_PEER);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_peer_connect_status(const uint8_t *address) {
    bt_channel_t *ch = peer_channel_for_address(address);
    return ch ? ch->peer_err : PBIO_ERROR_FAILED;
}

bool pbdrv_bluetooth_peer_is_connected(const uint8_t *address) {
    bt_channel_t *ch = peer_channel_for_address(address);
    return ch && peer_is_connected(ch);
}

uint32_t pbdrv_bluetooth_peer_get_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        count += peer_is_connected(&peer_channels[i]);
    }
    return count;
}

const uint8_t *pbdrv_bluetooth_peer_get_address(uint32_t index) {
    if (index >= PEER_NUM_CHANNELS || !peer_is_connected(&peer_channels[index])) {
        return NULL;
    }
    return peer_channels[index].addr;
}

void pbdrv_bluetooth_peer_get_local_address(uint8_t *address) {
    memcpy(address, bt.local_addr, 6);
}

void pbdrv_bluetooth_peer_disconnect(const uint8_t *address) {
    bt_channel_t *ch = peer_channel_for_address(address);
    if (!ch) {
        return;
    }
    bt_channel_close(ch);
    ch->peer_in_use = false;
    pbio_os_request_poll();
}

void pbdrv_bluetooth_peer_disconnect_all(void) {
    for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
        if (peer_channels[i].peer_in_use) {
            pbdrv_bluetooth_peer_disconnect(peer_channels[i].addr);
        }
    }
}

pbio_error_t pbdrv_bluetooth_peer_send(pbio_os_state_t *state, const uint8_t *dest_address, const uint8_t *data, uint32_t size) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (size > PBDRV_BLUETOOTH_PEER_MAX_MESSAGE_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (peer_tx_local.pending) {
        return PBIO_ERROR_BUSY;
    }
    if (pbdrv_bluetooth_peer_get_count() == 0) {
        return PBIO_ERROR_INVALID_OP;
    }

    uint32_t mask = 0;
    bt_channel_t *uplink = peer_channel_uplink();
    if (uplink) {
        // A peer sends everything to the coordinator, with the destination
        // in the header.
        mask = 1U << (uint32_t)(uplink - peer_channels);
        memcpy(peer_tx_local.address, dest_address, 6);
    } else {
        // The coordinator addresses its peers directly, with itself in the
        // header.
        for (uint32_t i = 0; i < PEER_NUM_CHANNELS; i++) {
            bt_channel_t *ch = &peer_channels[i];
            if (peer_is_connected(ch) && (peer_address_is_all(dest_address) || memcmp(ch->addr, dest_address, 6) == 0)) {
                mask |= 1U << i;
            }
        }
        if (!mask) {
            return PBIO_ERROR_NO_DEV;
        }
        memcpy(peer_tx_local.address, bt.local_addr, 6);
    }

    peer_tx_local.data = data;
    peer_tx_local.size = size;
    peer_tx_local.pending = mask;
    pbio_os_request_poll();

    PBIO_OS_AWAIT_UNTIL(state, peer_tx_local.pending == 0);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_peer_send_cancel(void) {
    peer_tx_local.pending = 0;
    peer_tx_local.data = NULL;
}

#endif // PBDRV_CONFIG_BLUETOOTH_PEER

static void bt_handle_message(uint8_t channel, const uint8_t *payload, uint16_t size) {

    #if DEBUG
    if (channel != BCSP_CHANNEL_L2CAP || size < 2 || bt_get_word(payload, 0) != L2CA_DATAREAD_IND) {
        DEBUG_PRINT("bt: rx ch%u:", channel);
        for (uint16_t i = 0; i < size && i < 16; i += 2) {
            DEBUG_PRINT(" %04x", bt_get_word(payload, i / 2));
        }
        DEBUG_PRINT(size > 16 ? " ... (%u)\n" : "\n", size);
    }
    #endif

    if (size < 2) {
        return;
    }
    uint16_t type = bt_get_word(payload, 0);

    if (channel == bt.awaited_channel && type == bt.awaited && !bt.received) {
        memset(bt.message, 0, sizeof(bt.message));
        for (uint16_t i = 0; i < size / 2 && i < BT_AWAITED_WORDS_MAX; i++) {
            bt.message[i] = bt_get_word(payload, i);
        }
        bt.received = true;
    }

    if (channel == BCSP_CHANNEL_RFCOMM) {
        bt_handle_rfcomm(type, payload, size);
    } else if (channel == BCSP_CHANNEL_DM) {
        bt_handle_dm(type, payload, size);
    }
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID || PBDRV_CONFIG_BLUETOOTH_PEER
    else if (channel == BCSP_CHANNEL_L2CAP) {
        bt_handle_l2cap(type, payload, size);
    }
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    else if (channel == BCSP_CHANNEL_SDP) {
        hid_handle_sdp(type, payload, size);
    }
    #endif
}

/**
 * Sends the next message that is due, if the link takes one: queued replies
 * first, then requests, then data.
 *
 * @return  Whether a message was sent.
 */
static bool bt_send_next(void) {
    if (!bcsp_can_send()) {
        return false;
    }
    if (bt.reply_count) {
        bt_reply_t *reply = &bt.replies[bt.reply_first];
        bt_send_message(reply->channel, reply->words, reply->size);
        bt.reply_first = (bt.reply_first + 1) % BT_REPLY_QUEUE_SIZE;
        bt.reply_count--;
        return true;
    }
    if (bt.shutdown_requested && bt.mux_open) {
        // Releasing only the channel leaves the multiplexer, and with it the
        // link, until the host closes it a few seconds later.
        const uint16_t msg[] = { RFC_CLOSE_REQ, bt.mux_id };
        bt_send_message(BCSP_CHANNEL_RFCOMM, msg, PBIO_ARRAY_SIZE(msg));
        bt.mux_open = false;
        bt_connection_reset();
        return true;
    }
    if (bt.connected && bt.disconnect_requested) {
        const uint16_t msg[] = { RFC_RELEASE_REQ, bt.mux_id, bt.server_chan };
        bt_send_message(BCSP_CHANNEL_RFCOMM, msg, PBIO_ARRAY_SIZE(msg));
        bt_connection_reset();
        return true;
    }
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID || PBDRV_CONFIG_BLUETOOTH_PEER
    if (bt_channel_send_request()) {
        return true;
    }
    #endif
    if (bt.scan_enable_due) {
        const uint16_t msg[] = {
            DM_HCI_WRITE_SCAN_ENABLE, 3, bt.discoverable ? BT_SCAN_INQUIRY_AND_PAGE : BT_SCAN_PAGE,
        };
        bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
        bt.scan_enable_due = false;
        DEBUG_PRINT("bt: discoverable %u\n", bt.discoverable);
        return true;
    }
    #if PBDRV_CONFIG_BLUETOOTH_INQUIRY
    if (inquiry_send_next()) {
        return true;
    }
    #endif
    // The inquiry is stopped for this by inquiry_send_next().
    if (bt.host_name_due && !bt.name_busy && !bt_inquiry_is_running()) {
        bt_remote_name_request(bt.host_name_addr);
        bt.host_name_due = false;
        return true;
    }
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    if (hid_send_next()) {
        return true;
    }
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_PEER
    if (peer_tx_pump()) {
        return true;
    }
    #endif
    // Return credits once the host is down to its last one, so that an
    // empty data message is not sent for every frame.
    if (bt.connected && bt.credit_flow && bt.rx_credits <= 1) {
        uint16_t credits = bt_take_credits_to_give();
        if (credits) {
            const uint16_t msg[] = { RFC_DATA_REQ, bt.mux_id, bt.server_chan, credits, 0, 0 };
            bt_send_message(BCSP_CHANNEL_RFCOMM, msg, PBIO_ARRAY_SIZE(msg));
            DEBUG_PRINT("bt: gave %u credits\n", credits);
            return true;
        }
    }
    return false;
}

/**
 * The SDP record for the Pybricks serial port: service class, protocols
 * (L2CAP, RFCOMM with our channel), public browse group, name. The BC4 adds
 * the record handle. The channel is filled in at index BT_SDP_CHANNEL_INDEX.
 */
static uint8_t bt_sdp_record[] = {
    // ServiceClassIDList: the Pybricks RFCOMM service class (128-bit UUID).
    0x09, 0x00, 0x01, 0x35, 0x11, 0x1c,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // ProtocolDescriptorList: L2CAP, RFCOMM channel.
    0x09, 0x00, 0x04, 0x35, 0x0c, 0x35, 0x03, 0x19, 0x01, 0x00,
    0x35, 0x05, 0x19, 0x00, 0x03, 0x08, 0x00,
    // BrowseGroupList: public browse root.
    0x09, 0x00, 0x05, 0x35, 0x03, 0x19, 0x10, 0x02,
    // ServiceName.
    0x09, 0x01, 0x00, 0x25, 0x08, 'P', 'y', 'b', 'r', 'i', 'c', 'k', 's',
};

#define BT_SDP_UUID_INDEX (6)
#define BT_SDP_CHANNEL_INDEX (38)

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID || PBDRV_CONFIG_BLUETOOTH_PEER
/** L2CAP PSMs that we take connections on. */
static const uint16_t bt_l2cap_psms[] = {
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    BT_PSM_HID_CONTROL,
    BT_PSM_HID_INTERRUPT,
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_PEER
    BT_PSM_PEER,
    #endif
};
#endif

/** Time to wait for the BC4 to answer a message. */
#define BT_REPLY_TIMEOUT_MS (1000)

/** Time to wait for the BCSP link to come up after the warm reset. */
#define BT_LINK_TIMEOUT_MS (3000)

/** Time to wait for the links to close on shutdown. */
#define BT_SHUTDOWN_TIMEOUT_MS (2000)

//
// Driver process.
//

static pbio_os_process_t bt_process;

/**
 * Holds the BC4 in reset. It is not powered off with the rest of the brick,
 * so it would otherwise keep its links and take new ones while the brick is
 * off.
 */
static void bt_power_off(void) {
    *AT91C_PIOA_CODR = BT_RST_PIN;
    bt.ready = false;
    pbio_busy_count_down();
}

static pbio_error_t bt_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;
    static uint32_t i;
    pbio_error_t err;

    // Until the BC4 is up, there is nothing to close.
    if (bt.shutdown_requested && !bt.ready) {
        bt_power_off();
        return PBIO_SUCCESS;
    }

    bcsp_poll();

    PBIO_OS_ASYNC_BEGIN(state);

    // Chip select deasserted, MISO without a pull-up since it is behind 10k.
    *AT91C_PIOA_PER = BT_RST_PIN | BT_ARM_CMD_PIN | BT_SPI_CS_PIN;
    *AT91C_PIOA_PPUDR = BT_ARM_CMD_PIN | BT_SPI_MISO_PIN;
    *AT91C_PIOA_SODR = BT_SPI_CS_PIN;
    *AT91C_PIOA_CODR = BT_ARM_CMD_PIN;
    *AT91C_PIOA_OER = BT_RST_PIN | BT_ARM_CMD_PIN | BT_SPI_CS_PIN;

restart:
    bcsp.link = BCSP_LINK_OFF;
    bt.ready = false;
    bt.reply_count = 0;
    bt.mux_open = false;
    bt.acl_count = 0;
    bt.name_busy = false;
    bt.host_name_due = false;
    bt_connection_reset();
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID || PBDRV_CONFIG_BLUETOOTH_PEER
    bt_channels_reset();
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_INQUIRY
    inquiry_reset();
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    hid_reset();
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_PEER
    peer_reset();
    #endif

    // Start from LEGO mode, with a clean RAM store.
    *AT91C_PIOA_CODR = BT_RST_PIN;
    PBIO_OS_AWAIT_MS(state, &timer, 10);
    *AT91C_PIOA_SODR = BT_RST_PIN;

    // Wait for its firmware to serve the mailbox, by asking for its build.
    PBIO_OS_AWAIT_MS(state, &timer, 500);
    for (i = 0;; i++) {
        uint16_t pdu[BCCMD_PDU_WORDS] = { BCCMD_GETREQ, BCCMD_PDU_WORDS, 0, BCCMD_VARID_BUILDID };
        if (bc4_bccmd(pdu, true) == PBIO_SUCCESS) {
            DEBUG_PRINT("bt: bc4 build %04x\n", pdu[5]);
            break;
        }
        if (i == 30) {
            DEBUG_PRINT("bt: no answer over SPI\n");
            return PBIO_ERROR_TIMEDOUT;
        }
        PBIO_OS_AWAIT_MS(state, &timer, 100);
    }

    for (i = 0; i < PBIO_ARRAY_SIZE(bc4_mode_keys); i++) {
        // Words: type, length, seq, varid, status, key, value size, store,
        // value.
        uint16_t pdu[BCCMD_PDU_WORDS_MAX] = {
            BCCMD_SETREQ, BCCMD_PDU_WORDS - 1 + bc4_mode_keys[i][1], i, BCCMD_VARID_PS, 0,
            bc4_mode_keys[i][0], bc4_mode_keys[i][1], BCCMD_PS_STORE_RAM, bc4_mode_keys[i][2], bc4_mode_keys[i][3],
        };
        err = bc4_bccmd(pdu, true);
        if (err != PBIO_SUCCESS) {
            DEBUG_PRINT("bt: setting &%04x failed\n", bc4_mode_keys[i][0]);
            return err;
        }
    }

    // The BC4 starts sending SYNC right after the warm reset, so be ready.
    memset(&bcsp, 0, sizeof(bcsp));
    uart_open();
    {
        uint16_t pdu[BCCMD_PDU_WORDS] = { BCCMD_SETREQ, BCCMD_PDU_WORDS, 0, BCCMD_VARID_WARM_RESET };
        bc4_bccmd(pdu, false);
    }
    bcsp_link_start();

    pbio_os_timer_set(&timer, BT_LINK_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, bcsp.link == BCSP_LINK_ACTIVE || pbio_os_timer_is_expired(&timer));
    if (bcsp.link != BCSP_LINK_ACTIVE) {
        DEBUG_PRINT("bt: no BCSP link, errors %u\n", uart_rx_errors);
        goto restart;
    }
    DEBUG_PRINT("bt: link active\n");

    // Register with the DM so that its results come to us.
    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        static const uint16_t msg[] = { DM_AM_REGISTER_REQ, BT_PHANDLE };
        bt_await(BCSP_CHANNEL_DM, DM_AM_REGISTER_CFM);
        bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
    }
    pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, bt.received || pbio_os_timer_is_expired(&timer));
    if (!bt.received) {
        DEBUG_PRINT("bt: DM register not confirmed\n");
        goto restart;
    }

    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        static const uint16_t msg[] = { DM_HCI_READ_BD_ADDR, 2 };
        bt_await(BCSP_CHANNEL_DM, DM_HCI_READ_BD_ADDR_COMPLETE);
        bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
    }
    pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, bt.received || pbio_os_timer_is_expired(&timer));
    if (!bt.received || bt.message[2] != 0) {
        DEBUG_PRINT("bt: no local address\n");
        goto restart;
    }
    {
        // Words: type, phandle, status, lap high, lap low, uap, nap.
        const uint16_t *m = bt.message;
        const uint8_t addr[] = { m[6] >> 8, m[6], m[5], m[3], m[4] >> 8, m[4] };
        pbdrv_hardware_set_mac_address(addr);
        memcpy(bt.local_addr, addr, sizeof(bt.local_addr));
        DEBUG_PRINT("bt: address %02x:%02x:%02x:%02x:%02x:%02x\n",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    }

    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        static const uint16_t msg[] = {
            DM_HCI_WRITE_CLASS_OF_DEVICE, 4, BT_CLASS_OF_DEVICE >> 16, BT_CLASS_OF_DEVICE & 0xffff,
        };
        bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
    }

    // Link policy for new links, incoming and outgoing. Without it, the
    // gamepad link cannot go into sniff mode.
    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        static const uint16_t msg[] = { DM_SET_DEFAULT_LINK_POLICY, BT_LINK_POLICY, BT_LINK_POLICY };
        bt_send_message(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg));
    }

    // The name goes in up to eight blocks the message points to. One will do.
    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        const char *name = pbsys_host_get_hub_display_name();
        uint16_t size = strlen(name) + 1;
        const uint16_t msg[] = { DM_HCI_CHANGE_LOCAL_NAME, size + 10, 0, 0, 0, 0, 0, 0, 0, 0 };
        bt_send_message_with_data(BCSP_CHANNEL_DM, msg, PBIO_ARRAY_SIZE(msg), (const uint8_t *)name, size);
        DEBUG_PRINT("bt: name %s\n", name);
    }

    // RFCOMM only takes connections once initialised, which registers it with
    // L2CAP. Words: type, phandle, PSM, use flow control, then flow control
    // settings (type, threshold, timer (2)) and two reserved values (2 each),
    // unused without it.
    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        static const uint16_t msg[] = { RFC_INIT_REQ, BT_PHANDLE, BT_PSM_RFCOMM, false, 0, 0, 0, 0, 0, 0, 0, 0 };
        bt_await(BCSP_CHANNEL_RFCOMM, RFC_INIT_CFM);
        bt_send_message(BCSP_CHANNEL_RFCOMM, msg, PBIO_ARRAY_SIZE(msg));
    }
    pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, bt.received || pbio_os_timer_is_expired(&timer));
    if (!bt.received) {
        DEBUG_PRINT("bt: RFCOMM init not confirmed\n");
        goto restart;
    }

    // RFCOMM picks our server channel.
    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        static const uint16_t msg[] = { RFC_REGISTER_REQ, BT_PHANDLE };
        bt_await(BCSP_CHANNEL_RFCOMM, RFC_REGISTER_CFM);
        bt_send_message(BCSP_CHANNEL_RFCOMM, msg, PBIO_ARRAY_SIZE(msg));
    }
    pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, bt.received || pbio_os_timer_is_expired(&timer));
    if (!bt.received || !bt.message[3]) {
        DEBUG_PRINT("bt: no RFCOMM channel\n");
        goto restart;
    }

    // Publish the serial port on that channel.
    PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
    {
        memcpy(&bt_sdp_record[BT_SDP_UUID_INDEX], pbio_pybricks_rfcomm_service_class_uuid, 16);
        bt_sdp_record[BT_SDP_CHANNEL_INDEX] = bt.server_chan;
        // Words: type, phandle, record pointer, record size.
        const uint16_t msg[] = { SDS_REGISTER_REQ, BT_PHANDLE, 0, sizeof(bt_sdp_record) };
        bt_await(BCSP_CHANNEL_SDP, SDS_REGISTER_CFM);
        bt_send_message_with_data(BCSP_CHANNEL_SDP, msg, PBIO_ARRAY_SIZE(msg), bt_sdp_record, sizeof(bt_sdp_record));
    }
    pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
    PBIO_OS_AWAIT_UNTIL(state, bt.received || pbio_os_timer_is_expired(&timer));
    // Words: type, phandle, record handle (2), result.
    DEBUG_PRINT("bt: SDP record %s, result %u, channel %u\n",
        bt.received ? "registered" : "not confirmed", bt.message[4], bt.server_chan);

    // Listen on the PSMs of HID and the hub network.
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID || PBDRV_CONFIG_BLUETOOTH_PEER
    for (i = 0; i < PBIO_ARRAY_SIZE(bt_l2cap_psms); i++) {
        PBIO_OS_AWAIT_UNTIL(state, bcsp_can_send());
        {
            // Words: type, PSM, phandle.
            const uint16_t msg[] = { L2CA_REGISTER_REQ, bt_l2cap_psms[i], BT_PHANDLE };
            bt_await(BCSP_CHANNEL_L2CAP, L2CA_REGISTER_CFM);
            bt_send_message(BCSP_CHANNEL_L2CAP, msg, PBIO_ARRAY_SIZE(msg));
        }
        pbio_os_timer_set(&timer, BT_REPLY_TIMEOUT_MS);
        PBIO_OS_AWAIT_UNTIL(state, bt.received || pbio_os_timer_is_expired(&timer));
        if (!bt.received) {
            DEBUG_PRINT("bt: L2CAP PSM %04x not registered\n", bt_l2cap_psms[i]);
            goto restart;
        }
    }
    #endif

    // Always page scan, so bonded devices can connect. Inquiry scan only
    // while discoverable, which is kept across a BC4 restart.
    bt.scan_enable_due = true;
    bt.ready = true;
    DEBUG_PRINT("bt: ready\n");

    // From here on, received messages are handled as they come in, and
    // whatever is due is sent as the link takes it.
    while (!bt.shutdown_requested) {
        PBIO_OS_AWAIT_UNTIL(state, bcsp.link != BCSP_LINK_ACTIVE || bt.shutdown_requested || bt_send_next());
        if (bcsp.link != BCSP_LINK_ACTIVE) {
            DEBUG_PRINT("bt: link lost\n");
            goto restart;
        }
    }

    // Close everything first, so the other ends know right away instead of
    // when their links time out. Their links go once no channel uses them.
    // The host's multiplexer is closed by bt_send_next().
    DEBUG_PRINT("bt: shutting down, %u links\n", bt.acl_count);
    #if PBDRV_CONFIG_BLUETOOTH_PEER
    pbdrv_bluetooth_peer_disconnect_all();
    #endif
    #if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
    // Also drops the link, so the gamepad turns off instead of waiting for
    // the link to time out.
    pbdrv_bluetooth_classic_hid_disconnect();
    #endif
    pbio_os_timer_set(&timer, BT_SHUTDOWN_TIMEOUT_MS);
    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, !bt.acl_count || bcsp.link != BCSP_LINK_ACTIVE ||
            pbio_os_timer_is_expired(&timer) || bt_send_next());
        if (!bt.acl_count || bcsp.link != BCSP_LINK_ACTIVE || pbio_os_timer_is_expired(&timer)) {
            break;
        }
    }
    DEBUG_PRINT("bt: off, %u links left\n", bt.acl_count);
    bt_power_off();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_init(void) {
    lwrb_init(&bt_rx_ring, bt_rx_ring_buf, sizeof(bt_rx_ring_buf));
    pbio_os_process_start(&bt_process, bt_process_thread, NULL);
}

void pbdrv_bluetooth_deinit(void) {
    bt.shutdown_requested = true;
    if (bt_process.err != PBIO_ERROR_AGAIN) {
        // The driver gave up at boot, so it will not run to do this.
        *AT91C_PIOA_CODR = BT_RST_PIN;
        return;
    }
    pbio_busy_count_up();
    pbio_os_request_poll();
}

//
// Host connection, over RFCOMM.
//

bool pbdrv_bluetooth_classic_host_is_connected(void) {
    return bt.connected;
}

const char *pbdrv_bluetooth_classic_host_get_connected_name(void) {
    return bt.connected ? pbio_bluetooth_classic_link_key_get_name(bt.host_addr) : NULL;
}

void pbdrv_bluetooth_classic_host_disconnect(void) {
    if (bt.connected) {
        bt.disconnect_requested = true;
        pbio_os_request_poll();
    }
}

uint32_t pbdrv_bluetooth_classic_host_rx_read(uint8_t *data, uint32_t size) {
    uint32_t read = lwrb_read(&bt_rx_ring, data, size);
    if (read) {
        // There may be room to let the host send more.
        pbio_os_request_poll();
    }
    return read;
}

pbio_error_t pbdrv_bluetooth_classic_host_tx_message(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {

    static uint32_t done;
    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    for (done = 0; done < size;) {
        // Replies to the BC4 go first, then one frame per credit. If the host
        // stops taking data, drop it rather than hold up the other transports
        // that get the same events.
        pbio_os_timer_set(&timer, BT_TX_TIMEOUT_MS);
        PBIO_OS_AWAIT_UNTIL(state, !bt.connected || pbio_os_timer_is_expired(&timer) ||
            (bcsp_can_send() && !bt.reply_count && bt.tx_unconfirmed < BT_TX_UNCONFIRMED_MAX &&
                (!bt.credit_flow || bt.tx_credits)));
        if (!bt.connected) {
            return PBIO_ERROR_INVALID_OP;
        }
        if (pbio_os_timer_is_expired(&timer)) {
            DEBUG_PRINT("bt: tx stalled, credits tx %u, can send %u, replies %u, unconfirmed %u\n",
                bt.tx_credits, bcsp_can_send(), bt.reply_count, bt.tx_unconfirmed);
            pbdrv_bluetooth_classic_host_disconnect();
            return PBIO_ERROR_TIMEDOUT;
        }
        uint32_t chunk = size - done;
        if (chunk > bt.frame_size) {
            chunk = bt.frame_size;
        }
        // Confirmed once sent on, unlike RFC_DATA_REQ.
        const uint16_t msg[] = { RFC_DATAWRITE_REQ, bt.mux_id, bt.server_chan, bt_take_credits_to_give(), chunk, 0 };
        bt_send_message_with_data(BCSP_CHANNEL_RFCOMM, msg, PBIO_ARRAY_SIZE(msg), &data[done], chunk);
        bt.tx_unconfirmed++;
        if (bt.credit_flow) {
            bt.tx_credits--;
        }
        done += chunk;
        DEBUG_PRINT("bt: tx %u, credits tx %u rx %u, unconfirmed %u\n", chunk, bt.tx_credits, bt.rx_credits, bt.tx_unconfirmed);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_classic_host_set_discoverable(bool discoverable) {
    // Sent once the BC4 takes it, so this is fine to call at any time.
    if (discoverable == bt.discoverable) {
        return;
    }
    bt.discoverable = discoverable;
    bt.scan_enable_due = true;
    pbio_os_request_poll();
}

bool pbdrv_bluetooth_classic_host_is_discoverable(void) {
    return bt.discoverable;
}

uint32_t pbdrv_bluetooth_classic_host_get_pair_count(void) {
    return bt.pair_count;
}

const char *pbdrv_bluetooth_classic_host_get_paired_name(void) {
    return bt.pair_count ? pbio_bluetooth_classic_link_key_get_name(bt.paired_addr) : NULL;
}

#endif // PBDRV_CONFIG_BLUETOOTH_NXT

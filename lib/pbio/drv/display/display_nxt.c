// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2007 the NxOS developers
// See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
// Copyright (c) 2025 The Pybricks Authors
//
// This driver contains a basic SPI driver to talk to the UltraChip
// 1601 LCD controller, as well as a higher level API implementing the
// UC1601's commandset.
//
// Note that the SPI driver is not suitable as a general-purpose SPI
// driver: the MISO pin (Master-In Slave-Out) is instead wired to the
// UC1601's CD input (used to select whether the transferred data is
// control commands or display data). Thus, the SPI driver here takes
// manual control of the MISO pin, and drives it depending on the type
// of data being transferred.
//
// This also means that you can only write to the UC1601, not read
// back from it. This is not too much of a problem, as we can just
// introduce a little delay in the places where we really need it.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_DISPLAY_NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/display.h>

#include <pbio/busy_count.h>
#include <pbio/os.h>

#include <at91sam7s256.h>

#include "nxos/lock.h"
#include "nxos/interrupts.h"
#include "nxos/drivers/systick.h"
#include "nxos/drivers/aic.h"

/*
 * Internal command bytes implementing part of the basic command set of
 * the UC1601.
 */
#define SET_COLUMN_ADDR0(addr) (0x00 | (addr & 0xF))
#define SET_COLUMN_ADDR1(addr) (0x10 | ((addr >> 4) & 0xF))
#define SET_MULTIPLEX_RATE(rate) (0x20 | (rate & 3))
#define SET_SCROLL_LINE(sl) (0x40 | (sl & 0x3F))
#define SET_PAGE_ADDR(page) (0xB0 | (page & 0xF))
#define SET_BIAS_POT0() (0x81)
#define SET_BIAS_POT1(pot) (pot & 0xFF)
#define SET_RAM_ADDR_CONTROL(auto_wrap, page_first, neg_inc, write_only_inc) \
    (0x88 | (auto_wrap << 0) | (page_first << 1) |                    \
    (neg_inc << 2) | (write_only_inc << 3))
#define SET_ALL_PIXELS_ON(on) (0xA4 | (on & 1))
#define SET_INVERSE_DISPLAY(on) (0xA6 | (on & 1))
#define ENABLE(on) (0xAE | (on & 1))
#define SET_MAP_CONTROL(mx, my) (0xC0 | (mx << 1) | (my << 2))
#define RESET() (0xE2)
#define SET_BIAS_RATIO(bias) (0xE8 | (bias & 3))

/*
 * SPI mode, command or data.
 */
typedef enum {
    /* Command. */
    SPI_MODE_COMMAND,
    /* Data. */
    SPI_MODE_DATA,
} spi_mode_t;

/*
 * SPI state.
 */
typedef enum {
    /* Operation started, bus is busy. */
    SPI_STATE_WAIT,
    /* Operation complete, bus is idle. */
    SPI_STATE_COMPLETE,
} spi_state_t;

/*
 * Current SPI mode, set with spi_set_tx_mode().
 */
static spi_mode_t spi_mode;

/*
 * Current SPI state.
 */
static volatile spi_state_t spi_state;

/*
 * User frame buffer. Each value is one pixel with value:
 *
 *  0: Empty / White
 *  1: Black
 */
static uint8_t pbdrv_display_user_frame[PBDRV_CONFIG_DISPLAY_NUM_ROWS][PBDRV_CONFIG_DISPLAY_NUM_COLS]
__attribute__((section(".noinit")));

/*
 * Flag to indicate that the user frame has been updated and needs to be
 * encoded and sent to the display driver.
 */
static bool pbdrv_display_user_frame_update_requested;

/*
 * Buffer to send one page of pixels to the display, using its internal
 * format. Every byte describes a column of 8 pixels, where the least
 * significant bit is used for the top pixel and the most significant bit is
 * used for the bottom pixel. If 1, the pixel is lit, or black. If 0, the
 * pixel is not lit, or white.
 */
static uint8_t pbdrv_display_send_buffer[PBDRV_CONFIG_DISPLAY_NUM_COLS];

/*
 * Set the data transmission mode.
 */
static void spi_set_tx_mode(spi_mode_t mode) {
    if (spi_mode == mode) {
        // Mode hasn't changed, no-op.
        return;
    } else {
        // If there is a mode switch, we need to let the SPI controller
        // drain all data first, to avoid spurious writes of the wrong
        // type.
        while (!(*AT91C_SPI_SR & AT91C_SPI_TXEMPTY)) {
            ;
        }
    }

    spi_mode = mode;

    if (mode == SPI_MODE_COMMAND) {
        *AT91C_PIOA_CODR = AT91C_PA12_MISO;
    } else {
        *AT91C_PIOA_SODR = AT91C_PA12_MISO;
    }
}

/*
 * Send a command byte to the LCD controller.
 */
static void spi_write_command_byte(uint8_t command) {
    spi_set_tx_mode(SPI_MODE_COMMAND);

    // Wait for the transmit register to empty.
    while (!(*AT91C_SPI_SR & AT91C_SPI_TDRE)) {
        ;
    }

    // Send the command byte and wait for a reply.
    *AT91C_SPI_TDR = command;
}

/*
 * Interrupt routine, called when Tx is done.
 */
static void spi_isr(void) {
    // Disable interrupts, this event is handled in thread context.
    *AT91C_SPI_IDR = AT91C_SPI_ENDTX;
    // Signal the thread context.
    spi_state = SPI_STATE_COMPLETE;
    pbio_os_request_poll();
}

static void spi_init(void) {
    uint32_t state = nx_interrupts_disable();

    spi_mode = SPI_MODE_COMMAND;
    spi_state = SPI_STATE_COMPLETE;

    // Enable power to the SPI and PIO controllers.
    *AT91C_PMC_PCER = (1 << AT91C_ID_SPI) | (1 << AT91C_ID_PIOA);

    // Configure the PIO controller: Hand the MOSI (Master Out, Slave
    // In) and SPI clock pins over to the SPI controller, but keep MISO
    // (Master In, Slave Out) and PA10 (Chip Select in this case) and
    // configure them for manually driven output.
    //
    // The initial configuration is command mode (sending LCD commands)
    // and the LCD controller chip not selected.
    *AT91C_PIOA_PDR = AT91C_PA13_MOSI | AT91C_PA14_SPCK;
    *AT91C_PIOA_ASR = AT91C_PA13_MOSI | AT91C_PA14_SPCK;

    *AT91C_PIOA_PER = AT91C_PA12_MISO | AT91C_PA10_NPCS2;
    *AT91C_PIOA_OER = AT91C_PA12_MISO | AT91C_PA10_NPCS2;
    *AT91C_PIOA_CODR = AT91C_PA12_MISO;
    *AT91C_PIOA_SODR = AT91C_PA10_NPCS2;

    // Disable all SPI interrupts, then configure the SPI controller in
    // master mode, with the chip select locked to chip 0 (UC1601 LCD
    // controller), communication at 2MHz, 8 bits per transfer and an
    // inactive-high clock signal.
    *AT91C_SPI_CR = AT91C_SPI_SWRST;
    *AT91C_SPI_CR = AT91C_SPI_SPIEN;
    *AT91C_SPI_IDR = ~0;
    *AT91C_SPI_MR = (6 << 24) | AT91C_SPI_MSTR;
    AT91C_SPI_CSR[0] = ((0x18 << 24) | (0x18 << 16) | (0x18 << 8) |
        AT91C_SPI_BITS_8 | AT91C_SPI_CPOL);

    // Now that the SPI bus is initialized, pull the Chip Select line
    // low, to select the uc1601. For some reason, letting the SPI
    // controller do this fails. Therefore, we force it once now.
    *AT91C_PIOA_CODR = AT91C_PA10_NPCS2;

    // Install an interrupt handler for the SPI controller, and enable
    // DMA transfers for SPI data. All SPI-related interrupt sources are
    // inhibited, so it won't bother us until we're ready.
    nx_aic_install_isr(AT91C_ID_SPI, AIC_PRIO_DRIVER, AIC_TRIG_LEVEL, spi_isr);
    *AT91C_SPI_PTCR = AT91C_PDC_TXTEN;

    nx_interrupts_enable(state);
}

void pbdrv_display_nxt_convert_page(int page) {
    int x, y;
    for (x = 0; x < PBDRV_CONFIG_DISPLAY_NUM_COLS; x++) {
        uint8_t b = 0;
        for (y = 0; y < 8; y++) {
            if (pbdrv_display_user_frame[page * 8 + y][x]) {
                b |= 1 << y;
            }
        }
        pbdrv_display_send_buffer[x] = b;
    }
}

static pbio_os_process_t pbdrv_display_nxt_process;

/*
 * Display driver process. Initialize the display and updates the display with
 * the user frame buffer if updated.
 */
static pbio_error_t pbdrv_display_nxt_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;
    static size_t i;
    static int page;

    // This is the command byte sequence that should be sent to the LCD
    // after a reset.
    static const uint8_t lcd_init_sequence[] = {
        // LCD power configuration.
        //
        // The LEGO Hardware Developer Kit documentation specifies that the
        // display should be configured with a multiplex rate (MR) of 1/65,
        // and a bias ratio (BR) of 1/9, and a display voltage V(LCD) of 9V.
        //
        // The specified MR and BR both map to simple command writes. V(LCD)
        // however is determined by an equation that takes into account both
        // the BR and the values of the PM (Potentiometer) and TC
        // (Temperature Compensation) configuration parameters.
        //
        // The equation and calculations required are a little too complex
        // to inline here, but the net result is that we should set a PM
        // value of 92. This will result in a smooth voltage gradient, from
        // 9.01V at -20 degrees Celsius to 8.66V at 60 degrees Celsius
        // (close to the maximum operational range of the LCD display).
        SET_MULTIPLEX_RATE(3),
        SET_BIAS_RATIO(3),
        SET_BIAS_POT0(),
        SET_BIAS_POT1(92),

        // Set the RAM address control, which defines how the data we send
        // to the LCD controller are placed in its internal video RAM.
        //
        // We want the bytes we send to be written in row-major order (line
        // by line), with no automatic wrapping.
        SET_RAM_ADDR_CONTROL(1, 0, 0, 0),

        // Set the LCD mapping mode, which defines how the data in video
        // RAM is driven to the display. The display on the NXT is mounted
        // upside down, so we want just Y mirroring.
        SET_MAP_CONTROL(0, 1),

        // Set the initial position of the video memory cursor. We
        // initialize it to point to the start of the screen.
        SET_COLUMN_ADDR0(0),
        SET_COLUMN_ADDR1(0),
        SET_PAGE_ADDR(0),

        // Turn the display on.
        ENABLE(1),
    };

    PBIO_OS_ASYNC_BEGIN(state);

    // Initialize the SPI controller to enable communication, then wait
    // a little bit for the UC1601 to register the new SPI bus state.
    spi_init();
    PBIO_OS_AWAIT_MS(state, &timer, 20);

    // Issue a reset command, and wait. Normally here we'd check the
    // UC1601 status register, but as noted at the start of the file, we
    // can't read from the LCD controller due to the board setup.
    spi_write_command_byte(RESET());
    PBIO_OS_AWAIT_MS(state, &timer, 20);

    // Send every command of the init sequence.
    for (i = 0; i < sizeof(lcd_init_sequence); i++) {
        spi_write_command_byte(lcd_init_sequence[i]);
    }

    // Clear display to start with.
    memset(&pbdrv_display_user_frame, 0, sizeof(pbdrv_display_user_frame));
    pbdrv_display_user_frame_update_requested = true;

    // Make sure that we are in data TX mode.
    spi_set_tx_mode(SPI_MODE_DATA);

    // Done initializing.
    pbio_busy_count_down();

    // Update the display with the user frame buffer, if changed.
    while (pbdrv_display_nxt_process.request != PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL) {
        PBIO_OS_AWAIT_UNTIL(state, pbdrv_display_user_frame_update_requested);
        pbdrv_display_user_frame_update_requested = false;
        for (page = 0; page < PBDRV_CONFIG_DISPLAY_NUM_ROWS / 8; page++) {
            // Convert pixel format.
            pbdrv_display_nxt_convert_page(page);
            // We are at the start of a page, so we need to send 100 bytes of
            // pixel data to display.
            spi_state = SPI_STATE_WAIT;
            *AT91C_SPI_TNPR = (uint32_t)pbdrv_display_send_buffer;
            *AT91C_SPI_TNCR = PBDRV_CONFIG_DISPLAY_NUM_COLS;
            *AT91C_SPI_IER = AT91C_SPI_ENDTX;
            PBIO_OS_AWAIT_UNTIL(state, spi_state == SPI_STATE_COMPLETE);
            // 100 bytes of displayable data have been transferred. We now
            // have to send 32 more bytes to get to the end of the page and
            // wrap around.
            //
            // Given that this data is off-screen, we just resend the first 32
            // bytes of the 100 we just transferred.
            spi_state = SPI_STATE_WAIT;
            *AT91C_SPI_TNPR = (uint32_t)pbdrv_display_send_buffer;
            *AT91C_SPI_TNCR = 132 - PBDRV_CONFIG_DISPLAY_NUM_COLS;
            *AT91C_SPI_IER = AT91C_SPI_ENDTX;
            PBIO_OS_AWAIT_UNTIL(state, spi_state == SPI_STATE_COMPLETE);
        }
    }

    // When power to the controller goes out, there is the risk that
    // some capacitors mounted around the controller might damage it
    // when discharging in an uncontrolled fashion. To avoid this, the
    // spec recommends putting the controller into reset mode before
    // shutdown, which activates a drain circuit to empty the board
    // capacitors gracefully.
    *AT91C_SPI_IDR = ~0;
    *AT91C_SPI_PTCR = AT91C_PDC_TXTDIS;
    spi_write_command_byte(RESET());
    PBIO_OS_AWAIT_MS(state, &timer, 20);

    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(PBIO_ERROR_CANCELED);
}

// Image corresponding to the display.
static pbio_image_t pbdrv_display_image;

void pbdrv_display_init(void) {
    // Initialize image.
    pbio_image_init(&pbdrv_display_image, (uint8_t *)pbdrv_display_user_frame,
        PBDRV_CONFIG_DISPLAY_NUM_COLS, PBDRV_CONFIG_DISPLAY_NUM_ROWS,
        PBDRV_CONFIG_DISPLAY_NUM_COLS);
    pbdrv_display_image.print_font = &pbio_font_mono_8x5_8;
    pbdrv_display_image.print_value = 1;

    // Start display process and ask pbdrv to wait until it is initialized.
    pbio_busy_count_up();
    pbio_os_process_start(&pbdrv_display_nxt_process, pbdrv_display_nxt_process_thread, NULL);
}

pbio_image_t *pbdrv_display_get_image(void) {
    return &pbdrv_display_image;
}

uint8_t pbdrv_display_get_max_value(void) {
    return 1;
}

uint8_t pbdrv_display_get_value_from_hsv(uint16_t h, uint8_t s, uint8_t v) {
    uint16_t l_x100 = v * (100 - s / 2);
    if (l_x100 < 50 * 100) {
        return 1;
    } else {
        return 0;
    }
}

void pbdrv_display_update(void) {
    pbdrv_display_user_frame_update_requested = true;
    pbio_os_request_poll();
}

void pbdrv_display_deinit(void) {
    // This doesn't do deinit right away, but it ask the display process to
    // gracefully exit at a useful spot, and then do deinit. The busy count is
    // used so all processes can await deinit before actual power off.
    pbio_busy_count_up();
    pbio_os_process_make_request(&pbdrv_display_nxt_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
}

#endif // PBDRV_CONFIG_DISPLAY_NXT

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors
//
// Display driver for ST7586S connected to EV3 via SPI.
//
// Based on TI evmAM1808 spi example. With additional inspiration from
// ev3dev and EV3RT display drivers.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_DISPLAY_EV3

#include <stdio.h>
#include <string.h>

#include <pbdrv/cache.h>
#include <pbdrv/display.h>
#include <pbdrv/gpio.h>

#include <pbio/busy_count.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include <tiam1808/edma.h>
#include <tiam1808/spi.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_edma3cc.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/edma_event.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#include "../drv/gpio/gpio_ev3.h"
#include <tiam1808/hw/hw_syscfg0_AM1808.h>

/* ST7586 Commands */
#define ST7586_NOP      (0x00)
#define ST7586_SWRESET  (0x01)
#define ST7586_SLPIN    (0x10)
#define ST7586_SLPOUT   (0x11)
#define ST7586_PTLON    (0x12)
#define ST7586_NORON    (0x13)
#define ST7586_INVOFF   (0x20)
#define ST7586_INVON    (0x21)
#define ST7586_APOFF    (0x22)
#define ST7586_APON     (0x23)
#define ST7586_DISPOFF  (0x28)
#define ST7586_DISPON   (0x29)
#define ST7586_CASET    (0x2A)
#define ST7586_RASET    (0x2B)
#define ST7586_RAMWR    (0x2C)
#define ST7586_RAMRD    (0x2E)
#define ST7586_PARAREA  (0x30)
#define ST7586_DSPCTL   (0x36)
#define ST7586_DSPGRAY  (0x38)
#define ST7586_DSPMONO  (0x39)
#define ST7586_RAMENB   (0x3A)
#define ST7586_DSPDUTY  (0xB0)
#define ST7586_PARDISP  (0xB4)
#define ST7586_NLNINV   (0xB5)
#define ST7586_VOP      (0xC0)
#define ST7586_BIAS     (0xC3)
#define ST7586_BOOST    (0xC4)
#define ST7586_VOPOFF   (0xC7)
#define ST7586_ANCTL    (0xD0)
#define ST7586_ARDCTL   (0xD7)
#define ST7586_OTPRWCTL (0xE0)
#define ST7586_OTPCOUT  (0xE1)
#define ST7586_OTPWR    (0xE2)
#define ST7586_OTPRD    (0xE3)

/**
 * st7586s action types
 */
typedef enum {
    /** Command */
    ST7586S_ACTION_WRITE_COMMAND,
    /** Data */
    ST7586S_ACTION_WRITE_DATA,
    /** Delay */
    ST7586S_ACTION_DELAY,
} pbdrv_display_st7586s_action_type_t;

typedef struct {
    pbdrv_display_st7586s_action_type_t type;
    uint32_t payload;
} pbdrv_display_st7586s_action_t;

/**
 * SPI bus state.
 */
typedef enum {
    /** Operation started, bus is busy. */
    SPI_STATUS_WAIT,
    /** Operation complete, bus is idle. */
    SPI_STATUS_COMPLETE,
    /** Operation failed. */
    SPI_STATUS_ERROR,
} spi_status_t;

static const pbdrv_gpio_t pin_spi1_mosi = PBDRV_GPIO_EV3_PIN(5, 23, 20, 2, 10);

static const pbdrv_gpio_t pin_spi1_clk = PBDRV_GPIO_EV3_PIN(5, 11, 8, 2, 13);

static const pbdrv_gpio_t pin_lcd_a0 = PBDRV_GPIO_EV3_PIN(5, 19, 16, 2, 11);

static const pbdrv_gpio_t pin_lcd_cs = PBDRV_GPIO_EV3_PIN(5, 15, 12, 2, 12);

static const pbdrv_gpio_t pin_lcd_reset = PBDRV_GPIO_EV3_PIN(12, 31, 28, 5, 0);

static volatile spi_status_t spi_status = SPI_STATUS_ERROR;

/**
 * Number of column triplets. Each triplet is 3 columns of pixels, as detailed
 * below in the description of the display buffer.
 */
#define ST7586S_NUM_COL_TRIPLETS ((PBDRV_CONFIG_DISPLAY_NUM_COLS + 2) / 3)

/**
 * Number of rows. This is the same as the number of display rows.
 */
#define ST7586S_NUM_ROWS (PBDRV_CONFIG_DISPLAY_NUM_ROWS)

/**
 * Maximum pixel value.
 */
#define ST7586S_VALUE_MAX 3

/**
 * User frame buffer. Each value is one pixel with value:
 *
 *  0: Empty / White
 *  1: Light Grey
 *  2: Dark Grey
 *  3: Black
 *
 * Non-atomic updated by the application are allowed.
 */
static uint8_t pbdrv_display_user_frame[PBDRV_CONFIG_DISPLAY_NUM_ROWS][ST7586S_NUM_COL_TRIPLETS * 3] __attribute__((section(".noinit"), used));

/**
 * Flag to indicate that the user frame has been updated and needs to be
 * encoded and sent to the display driver.
 */
static bool pbdrv_display_user_frame_update_requested;

/**
 * Display buffer in the format ready for sending to the st7586s display driver.
 *
 * Three pixels are encoded in one byte as  (MSB) | A B C | A B C | A B | (LSB)
 *
 * A  B (C)
 * --------------------
 * 0  0  0  Empty
 * 0  1  0  Light Grey
 * 1  0  0  Dark Grey
 * 1  1  1  Black
 *
 * Column C is essentially redundant, but required for the first and second
 * pixel in each triplet.
 *
 * The display driver supports up to 384 columns and 160 rows. Our display
 * is 178 x 128, so we use 128 rows and 60 triplets of columns. This spans
 * 180 columns, but we only use 178. The remaining 2 columns are not displayed.
 *
 * Even in monochrome mode, you can only have 3 pixels per byte, so there is no
 * savings in using it. We might as well support gray scale.
 */
static uint8_t st7586s_send_buf[ST7586S_NUM_COL_TRIPLETS * ST7586S_NUM_ROWS] __attribute__((section(".noinit"), used));

/**
 * Encode a triplet of pixels into a single byte in the format descrived above.
 *
 * @param p0 First pixel.
 * @param p1 Second pixel.
 * @param p2 Third pixel.
 *
 * @return Encoded triplet.
 */
static uint8_t encode_triplet(uint8_t p0, uint8_t p1, uint8_t p2) {
    // As described above, the first two pixels are the normal binary
    // representation shifted left by one, with an extra bit set for black.
    // The third pixel is not shifted, so contains just two bits.
    p0 = p0 >= ST7586S_VALUE_MAX ? 0b111 : (p0 << 1);
    p1 = p1 >= ST7586S_VALUE_MAX ? 0b111 : (p1 << 1);
    p2 = p2 >= ST7586S_VALUE_MAX ? 0b11 : p2;

    // Three pixels are then concatenated to one byte.
    return p0 << 5 | p1 << 2 | p2;
}

/**
 * Encode the user frame buffer into the display buffer.
 */
void pbdrv_display_st7586s_encode_user_frame(void) {
    // Iterating over display rows (and ST7586S rows are the same).
    for (size_t row = 0; row < PBDRV_CONFIG_DISPLAY_NUM_ROWS; row++) {
        // Iterating ST7586S column-triplets, which are 3 columns each.
        for (size_t triplet = 0; triplet < ST7586S_NUM_COL_TRIPLETS; triplet++) {
            uint8_t p0 = pbdrv_display_user_frame[row][triplet * 3];
            uint8_t p1 = pbdrv_display_user_frame[row][triplet * 3 + 1];
            uint8_t p2 = pbdrv_display_user_frame[row][triplet * 3 + 2];
            st7586s_send_buf[row * ST7586S_NUM_COL_TRIPLETS + triplet] = encode_triplet(p0, p1, p2);
        }
    }
}

/**
 * Whether to reset and initialize the display. We skip it since the EEPROM
 * bootloader already does this.
 */
#define ST7586S_DO_RESET_AND_INIT (0)

/**
 * Display initialization script adapted from ev3dev and EV3RT.
 */
static const pbdrv_display_st7586s_action_t init_script[] = {
    #if ST7586S_DO_RESET_AND_INIT
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_ARDCTL},
    { ST7586S_ACTION_WRITE_DATA, 0x9f},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_OTPRWCTL},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_DELAY, 10},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_OTPRD},
    { ST7586S_ACTION_DELAY, 20},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_OTPCOUT},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_SLPOUT},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_DISPOFF},
    { ST7586S_ACTION_DELAY, 50},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_VOPOFF},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_VOP},
    { ST7586S_ACTION_WRITE_DATA, 0xE3},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_BIAS},
    { ST7586S_ACTION_WRITE_DATA, 0x02},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_BOOST},
    { ST7586S_ACTION_WRITE_DATA, 0x04},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_ANCTL},
    { ST7586S_ACTION_WRITE_DATA, 0x1d},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_NLNINV},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_DSPMONO},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_RAMENB},
    { ST7586S_ACTION_WRITE_DATA, 0x02},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_DSPCTL},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_DSPDUTY},
    { ST7586S_ACTION_WRITE_DATA, 0x7f},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_PARDISP},
    { ST7586S_ACTION_WRITE_DATA, 0xa0},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_PARAREA},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_DATA, 0x77},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_INVOFF},
    { ST7586S_ACTION_DELAY, 100},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_DISPON},
    { ST7586S_ACTION_DELAY, 100},
    #endif // ST7586S_DO_RESET_AND_INIT
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_CASET},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    // This differs from the official firmware and ev3dev, which use 0x7f and
    // reset it to the value below on every new frame.
    { ST7586S_ACTION_WRITE_DATA, ST7586S_NUM_COL_TRIPLETS - 1},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_RASET},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    { ST7586S_ACTION_WRITE_DATA, 0x00},
    // This differs from the official firmware and ev3dev, which use 0x9f and
    // reset it to the value below on every new frame.
    { ST7586S_ACTION_WRITE_DATA, ST7586S_NUM_ROWS - 1},
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_DSPGRAY},
    // This differs from the official firmware and ev3dev, which write this on
    // every frame. By enteringthis mode right away we can keep sending new
    // frame data without switching back to command mode.
    { ST7586S_ACTION_WRITE_COMMAND, ST7586_RAMWR},
};

/**
 * Called on SPI1 DMA transfer completion.
 *
 * @param [in] status Status of the transfer.
 */
void pbdrv_display_ev3_spi1_tx_complete(uint32_t status) {
    SPIIntDisable(SOC_SPI_1_REGS, SPI_DMA_REQUEST_ENA_INT);
    spi_status = SPI_STATUS_COMPLETE;
    pbio_os_request_poll();
}

/**
 * Begin writing data to the display via DMA.
 *
 * @param [in] data Data to write.
 * @param [in] size Size of the data.
 */
void pbdrv_display_st7586s_write_data_begin(uint8_t *data, uint32_t size) {
    spi_status = SPI_STATUS_WAIT;
    pbdrv_gpio_out_low(&pin_lcd_cs);

    pbdrv_cache_prepare_before_dma(data, size);

    // Parameter object must be volatile since it is copied byte-by-byte in the
    // TI API, causing it to be optimized out.
    volatile EDMA3CCPaRAMEntry paramSet = {
        // Address of the data to be sent.
        .srcAddr = (uint32_t)data,
        // Address of the destination register.
        .destAddr = SOC_SPI_1_REGS + SPI_SPIDAT1,
        // Number of bytes in an array.
        .aCnt = 1,
        // Number of such arrays to be transferred.
        .bCnt = size,
        // Number of frames of aCnt*bBcnt bytes to be transferred.
        .cCnt = 1,
        // The srcBidx should be incremented by aCnt number of bytes since the
        // source used here is memory.
        .srcBIdx = 1,
        // A sync Transfer Mode is set in OPT. srCIdx and destCIdx set to
        // zero since ASYNC Mode is used.
        .srcCIdx = 0,
        // Linking transfers in EDMA3 are not used.
        .linkAddr = 0xFFFF,
        // bCntReload is not used.
        .bCntReload = 0,
        // Options for the transfer. SAM field in OPT is set to zero since
        // source is memory and memory pointer needs to be incremented. DAM
        // field in OPT is set to zero since destination is not a FIFO.
        .opt = (
            // Transfer completion code.
            ((EDMA3_CHA_SPI1_TX << EDMA3CC_OPT_TCC_SHIFT) & EDMA3CC_OPT_TCC) |
            // EDMA3 Interrupt is enabled and Intermediate Interrupt Disabled.
            (1 << EDMA3CC_OPT_TCINTEN_SHIFT)
            ),
    };

    // Now write the PaRam Set to EDMA3.
    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI1_TX, (EDMA3CCPaRAMEntry *)&paramSet);

    // EDMA3 Transfer is Enabled.
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI1_TX, EDMA3_TRIG_MODE_EVENT);

    // Enable SPI controller to generate DMA events.
    SPIIntEnable(SOC_SPI_1_REGS, SPI_DMA_REQUEST_ENA_INT);
}

/**
 * Initialize the display SPI driver.
 *
 * Pinmux and common EDMA handlers are already set up in platform.c.
 */
static void pbdrv_display_ev3_spi_init(void) {

    // GPIO Mux. CS is in GPIO mode (manual control).
    pbdrv_gpio_alt(&pin_spi1_mosi, SYSCFG_PINMUX5_PINMUX5_23_20_SPI1_SIMO0);
    pbdrv_gpio_alt(&pin_spi1_clk, SYSCFG_PINMUX5_PINMUX5_11_8_SPI1_CLK);
    pbdrv_gpio_input(&pin_lcd_a0);
    pbdrv_gpio_input(&pin_lcd_cs);
    pbdrv_gpio_out_high(&pin_lcd_reset);

    // Waking up the SPI1 instance.
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_SPI1, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // Reset.
    SPIReset(SOC_SPI_1_REGS);
    SPIOutOfReset(SOC_SPI_1_REGS);

    // Mode.
    SPIModeConfigure(SOC_SPI_1_REGS, SPI_MASTER_MODE);
    SPIPinControlSet(SOC_SPI_1_REGS, 0, SPI_SPIPC0_SOMIFUN | SPI_SPIPC0_SIMOFUN | SPI_SPIPC0_CLKFUN | SPI_SPIPC0_ENAFUN);

    // Config.
    SPIClkConfigure(SOC_SPI_1_REGS, SOC_SYSCLK_2_FREQ, 10000000, SPI_DATA_FORMAT0);
    SPICharLengthSet(SOC_SPI_1_REGS, 8, SPI_DATA_FORMAT0);
    SPIConfigClkFormat(SOC_SPI_1_REGS, SPI_CLK_POL_HIGH, SPI_DATA_FORMAT0);
    SPIDelayConfigure(SOC_SPI_1_REGS, 0, 0, 10, 10);
    SPIIntLevelSet(SOC_SPI_1_REGS, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);

    // Request DMA Channel and TCC for SPI Transmit with queue number 0.
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, EDMA3_CHA_SPI1_TX, EDMA3_CHA_SPI1_TX, 0);

    // Enable the SPI controller.
    SPIEnable(SOC_SPI_1_REGS);
}

static pbio_os_process_t pbdrv_display_ev3_process;

/**
 * Display driver process. Initializes the display and updates the display
 * with the user frame buffer if the user data was updated.
 */
static pbio_error_t pbdrv_display_ev3_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;
    static uint32_t script_index;
    static uint8_t payload;

    PBIO_OS_ASYNC_BEGIN(state);

    #if ST7586S_DO_RESET_AND_INIT
    pbdrv_gpio_out_low(&pin_lcd_reset);
    PBIO_OS_AWAIT_MS(state, &timer, 10);
    pbdrv_gpio_out_high(&pin_lcd_reset);
    PBIO_OS_AWAIT_MS(state, &timer, 120);
    #endif // ST7586S_DO_RESET_AND_INIT

    // For every action in the init script, either send a command or data, or
    // wait for a given delay.
    for (script_index = 0; script_index < PBIO_ARRAY_SIZE(init_script); script_index++) {
        const pbdrv_display_st7586s_action_t *action = &init_script[script_index];

        if (action->type == ST7586S_ACTION_DELAY) {
            // Simple delay.
            PBIO_OS_AWAIT_MS(state, &timer, action->payload);
        } else {
            // Send command or data.
            payload = action->payload;
            if (action->type == ST7586S_ACTION_WRITE_DATA) {
                pbdrv_gpio_out_high(&pin_lcd_a0);
            } else {
                pbdrv_gpio_out_low(&pin_lcd_a0);
            }
            pbdrv_display_st7586s_write_data_begin(&payload, sizeof(payload));
            PBIO_OS_AWAIT_UNTIL(state, spi_status == SPI_STATUS_COMPLETE);
            pbdrv_gpio_out_high(&pin_lcd_cs);
        }
    }

    // Staying in data mode from here.
    pbdrv_gpio_out_high(&pin_lcd_a0);

    // Clear display to start with.
    memset(&pbdrv_display_user_frame, 0, sizeof(pbdrv_display_user_frame));
    pbdrv_display_user_frame_update_requested = true;

    // Done initializing.
    pbio_busy_count_down();

    // Update the display with the user frame buffer, if changed.
    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, pbdrv_display_user_frame_update_requested);
        pbdrv_display_user_frame_update_requested = false;
        pbdrv_display_st7586s_encode_user_frame();
        pbdrv_display_st7586s_write_data_begin(st7586s_send_buf, sizeof(st7586s_send_buf));
        PBIO_OS_AWAIT_UNTIL(state, spi_status == SPI_STATUS_COMPLETE);
        pbdrv_gpio_out_high(&pin_lcd_cs);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Image corresponding to the display.
 */
static pbio_image_t display_image;

/**
 * Initialize the display driver.
 */
void pbdrv_display_init(void) {
    // Initialize SPI.
    pbdrv_display_ev3_spi_init();

    // Initialize image.
    pbio_image_init(&display_image, (uint8_t *)pbdrv_display_user_frame,
        PBDRV_CONFIG_DISPLAY_NUM_COLS, PBDRV_CONFIG_DISPLAY_NUM_ROWS,
        ST7586S_NUM_COL_TRIPLETS * 3);
    display_image.print_font = &pbio_font_terminus_normal_16;
    display_image.print_value = ST7586S_VALUE_MAX;

    // Start display process and ask pbdrv to wait until it is initialized.
    pbio_busy_count_up();
    pbio_os_process_start(&pbdrv_display_ev3_process, pbdrv_display_ev3_process_thread, NULL);
}

pbio_image_t *pbdrv_display_get_image(void) {
    return &display_image;
}

uint8_t pbdrv_display_get_max_value(void) {
    return ST7586S_VALUE_MAX;
}

void pbdrv_display_update(void) {
    pbdrv_display_user_frame_update_requested = true;
    pbio_os_request_poll();
}

#endif // PBDRV_CONFIG_DISPLAY_EV3

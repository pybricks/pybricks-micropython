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

#include <contiki.h>

#include "../core.h"

#include <pbdrv/gpio.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include <tiam1808/spi.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
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

static const pbdrv_gpio_t pin_spi1_mosi = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 5,
        .mux_mask = SYSCFG_PINMUX5_PINMUX5_23_20,
        .mux_shift = SYSCFG_PINMUX5_PINMUX5_23_20_SHIFT,
        .gpio_bank_id = 2,
        .gpio_mode = SYSCFG_PINMUX5_PINMUX5_23_20_GPIO2_10,
    },
    .pin = 10,
};

static const pbdrv_gpio_t pin_spi1_clk = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 5,
        .mux_mask = SYSCFG_PINMUX5_PINMUX5_11_8,
        .mux_shift = SYSCFG_PINMUX5_PINMUX5_11_8_SHIFT,
        .gpio_bank_id = 2,
        .gpio_mode = SYSCFG_PINMUX5_PINMUX5_11_8_GPIO2_13,
    },
    .pin = 13,
};

static const pbdrv_gpio_t pin_lcd_a0 = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 5,
        .mux_mask = SYSCFG_PINMUX5_PINMUX5_19_16,
        .mux_shift = SYSCFG_PINMUX5_PINMUX5_19_16_SHIFT,
        .gpio_bank_id = 2,
        .gpio_mode = SYSCFG_PINMUX5_PINMUX5_19_16_GPIO2_11,
    },
    .pin = 11,
};

static const pbdrv_gpio_t pin_lcd_cs = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 5,
        .mux_mask = SYSCFG_PINMUX5_PINMUX5_15_12,
        .mux_shift = SYSCFG_PINMUX5_PINMUX5_15_12_SHIFT,
        .gpio_bank_id = 2,
        .gpio_mode = SYSCFG_PINMUX5_PINMUX5_15_12_GPIO2_12,
    },
    .pin = 12,
};

static const pbdrv_gpio_t pin_lcd_reset = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 12,
        .mux_mask = SYSCFG_PINMUX12_PINMUX12_31_28,
        .mux_shift = SYSCFG_PINMUX12_PINMUX12_31_28_SHIFT,
        .gpio_bank_id = 5,
        .gpio_mode = SYSCFG_PINMUX12_PINMUX12_31_28_GPIO5_0,
    },
    .pin = 0,
};

static volatile spi_status_t spi_status = SPI_STATUS_ERROR;

PROCESS(pbdrv_display_ev3_init_process, "st7586s");

/**
 * Number of column triplets. Each triplet is 3 columns of pixels, as detailed
 * below in the description of the display buffer.
 */
#define ST7586S_NUM_COL_TRIPLETS (60)

/**
 * Number of rows. This is the same as the number of display rows.
 */
#define ST7586S_NUM_ROWS (PBDRV_CONFIG_DISPLAY_NUM_ROWS)

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
static uint8_t pbdrv_display_user_frame[PBDRV_CONFIG_DISPLAY_NUM_ROWS][PBDRV_CONFIG_DISPLAY_NUM_COLS] __attribute__((section(".pbdrv_display_user_frame"), used));

/**
 * Flag to indicate that the user frame has been updated and needs to be
 * encoded and sent to the display driver.
 */
static bool pbdrv_display_user_frame_update_requested = false;

/**
 * Display buffer in the format ready for sending to the st7586s display driver.
 *
 * Three pixels are encoded in one byte as | A B C | A B C | A B
 *
 * A  B (C)
 * --------------------
 * 0  0  0  Empty
 * 1  0  0  Dark Grey
 * 0  1  0  Light Grey
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
static uint8_t st7586s_send_buf[ST7586S_NUM_COL_TRIPLETS * ST7586S_NUM_ROWS] __attribute__((section(".st7586s_send_buf"), used));

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
    if (p0 >= 3) {
        p0 = 7;
    }
    if (p1 >= 3) {
        p1 = 7;
    }
    if (p2 >= 3) {
        p2 = 7;
    }
    return p0 << 6 | p1 << 3 | p2;
}

/**
 * Splash boot logo for Pybricks. This is a 178x128 pixel monochrome image.
 * Starts at no color, and toggles to on at every index below.
 *
 * Revisit: Use standard format, e.g. decompress PNG files from file storage or memory card.
 */
static uint16_t pbdrv_display_pybricks_logo[] = {
    3227, 3359, 3403, 3539, 3580, 3718, 3757, 3897, 3934, 4076, 4111, 4255, 4289, 4433, 4466, 4612,
    4644, 4790, 4822, 4968, 5000, 5146, 5178, 5324, 5356, 5502, 5534, 5548, 5666, 5680, 5712, 5725,
    5845, 5858, 5890, 5903, 6023, 6036, 6068, 6081, 6201, 6214, 6246, 6259, 6379, 6392, 6424, 6437,
    6467, 6474, 6520, 6527, 6557, 6570, 6602, 6615, 6643, 6654, 6696, 6707, 6735, 6748, 6780, 6793,
    6820, 6833, 6873, 6886, 6913, 6926, 6958, 6971, 6997, 7012, 7050, 7065, 7091, 7104, 7136, 7149,
    7174, 7191, 7227, 7244, 7269, 7282, 7314, 7327, 7352, 7369, 7405, 7422, 7447, 7460, 7492, 7505,
    7530, 7548, 7582, 7601, 7625, 7638, 7670, 7683, 7707, 7726, 7760, 7779, 7803, 7816, 7848, 7861,
    7885, 7904, 7938, 7957, 7981, 7994, 8026, 8039, 8063, 8082, 8116, 8135, 8159, 8172, 8204, 8217,
    8241, 8260, 8294, 8313, 8337, 8350, 8382, 8395, 8419, 8438, 8472, 8491, 8515, 8528, 8560, 8573,
    8598, 8615, 8651, 8668, 8693, 8706, 8738, 8751, 8776, 8793, 8829, 8846, 8871, 8884, 8916, 8929,
    8955, 8970, 9008, 9023, 9049, 9062, 9094, 9107, 9134, 9148, 9186, 9201, 9227, 9240, 9272, 9285,
    9313, 9325, 9366, 9378, 9405, 9418, 9450, 9463, 9492, 9501, 9545, 9554, 9583, 9596, 9628, 9641,
    9673, 9676, 9726, 9729, 9761, 9774, 9806, 9819, 9939, 9952, 9984, 9997, 10117, 10130, 10162,
    10175, 10295, 10308, 10340, 10353, 10473, 10486, 10518, 10531, 10651, 10664, 10696, 10709,
    10829, 10842, 10874, 10887, 11007, 11020, 11052, 11065, 11185, 11198, 11230, 11243, 11363,
    11376, 11408, 11421, 11541, 11554, 11586, 11599, 11719, 11732, 11764, 11777, 11802, 11806,
    11815, 11819, 11828, 11832, 11841, 11846, 11855, 11859, 11868, 11872, 11897, 11910, 11942,
    11955, 11980, 11984, 11993, 11997, 12006, 12010, 12019, 12024, 12033, 12037, 12046, 12050,
    12075, 12088, 12120, 12133, 12158, 12162, 12171, 12175, 12184, 12188, 12197, 12202, 12211,
    12215, 12224, 12228, 12253, 12266, 12298, 12311, 12336, 12340, 12349, 12353, 12362, 12366,
    12375, 12380, 12389, 12393, 12402, 12406, 12431, 12444, 12476, 12489, 12514, 12518, 12527,
    12531, 12540, 12544, 12553, 12558, 12567, 12571, 12580, 12584, 12609, 12622, 12654, 12667,
    12681, 12774, 12787, 12800, 12832, 12845, 12859, 12952, 12965, 12978, 13010, 13023, 13037,
    13130, 13143, 13156, 13188, 13201, 13215, 13308, 13321, 13334, 13366, 13379, 13393, 13486,
    13499, 13512, 13544, 13557, 13571, 13664, 13677, 13690, 13722, 13735, 13749, 13842, 13855,
    13868, 13900, 13913, 13927, 14020, 14033, 14046, 14078, 14091, 14105, 14198, 14211, 14224,
    14256, 14269, 14283, 14376, 14389, 14402, 14434, 14447, 14461, 14554, 14567, 14580, 14612,
    14625, 14639, 14732, 14745, 14758, 14790, 14803, 14817, 14910, 14923, 14936, 14968, 14982,
    14994, 15088, 15100, 15114, 15146, 15186, 15252, 15292, 15324, 15364, 15430, 15470, 15502,
    15542, 15608, 15648, 15680, 15720, 15786, 15826, 15858, 15898, 15964, 16004, 16036, 16075,
    16143, 16182, 16215, 16253, 16321, 16359, 16393, 16431, 16499, 16537, 16572, 16608, 16678,
    16714, 16751, 16785, 16857, 16891, 16930, 16962, 17036, 17068, 17109, 17139, 17215, 17245,
    17289, 17314, 17396, 17421
};

/**
 * Load an image encoded in the format above into the user frame buffer.
 *
 * @param bitmap Pointer to the bitmap data.
 */
static void pbdrv_display_load_indexed_bitmap(uint16_t *indexed_bitmap) {
    bool set = 0;
    uint32_t switch_index = 0;
    for (size_t r = 0; r < PBDRV_CONFIG_DISPLAY_NUM_ROWS; r++) {
        for (size_t c = 0; c < PBDRV_CONFIG_DISPLAY_NUM_COLS; c++) {
            if ((r * PBDRV_CONFIG_DISPLAY_NUM_COLS + c) == indexed_bitmap[switch_index]) {
                set = !set;
                switch_index++;
            }
            pbdrv_display_user_frame[r][c] = set ? 3 : 0;
        }
    }
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
            // The last triplet has no second and third pixel.
            uint8_t p1 = triplet == ST7586S_NUM_COL_TRIPLETS - 1 ? 0 : pbdrv_display_user_frame[row][triplet * 3 + 1];
            uint8_t p2 = triplet == ST7586S_NUM_COL_TRIPLETS - 1 ? 0 : pbdrv_display_user_frame[row][triplet * 3 + 2];
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
static pbdrv_display_st7586s_action_t init_script[] = {
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

static uint32_t tx_size;
static uint32_t tx_progress;
static uint8_t *tx_data;

/**
 * SPI interrupt service routine for transmitting data.
 */
void SPIIsr(void) {
    uint32_t intCode = 0;
    IntSystemStatusClear(SYS_INT_SPINT1);

    while ((intCode = SPIInterruptVectorGet(SOC_SPI_1_REGS))) {
        if (intCode != SPI_TX_BUF_EMPTY) {
            continue;
        }

        SPITransmitData1(SOC_SPI_1_REGS, *tx_data++);

        if (++tx_progress == tx_size) {
            SPIIntDisable(SOC_SPI_1_REGS, SPI_TRANSMIT_INT);
            spi_status = SPI_STATUS_COMPLETE;
            process_poll(&pbdrv_display_ev3_init_process);
        }
    }
}

/**
 * Begin writing data to the display.
 *
 * @param data Data to write.
 * @param size Size of the data.
 */
void pbdrv_display_st7586s_write_data_begin(uint8_t *data, uint32_t size) {
    tx_data = data;
    tx_size = size;
    tx_progress = 0;
    spi_status = SPI_STATUS_WAIT;
    pbdrv_gpio_out_low(&pin_lcd_cs);
    SPIEnable(SOC_SPI_1_REGS);
    SPIIntEnable(SOC_SPI_1_REGS, SPI_TRANSMIT_INT);
}

/**
 * Initialize the display SPI driver. Pinmux is already set up in platform.c.
 */
void pbdrv_display_init(void) {

    // GPIO Mux. CS is in GPIO mode (manual control).
    pbdrv_gpio_alt(&pin_spi1_mosi, SYSCFG_PINMUX5_PINMUX5_23_20_SPI1_SIMO0);
    pbdrv_gpio_alt(&pin_spi1_clk, SYSCFG_PINMUX5_PINMUX5_11_8_SPI1_CLK);
    pbdrv_gpio_alt(&pin_lcd_a0, ((pbdrv_gpio_tiam1808_mux_t *)pin_lcd_a0.bank)->gpio_mode);
    pbdrv_gpio_alt(&pin_lcd_cs, ((pbdrv_gpio_tiam1808_mux_t *)pin_lcd_cs.bank)->gpio_mode);
    pbdrv_gpio_alt(&pin_lcd_reset, ((pbdrv_gpio_tiam1808_mux_t *)pin_lcd_reset.bank)->gpio_mode);

    // Waking up the SPI1 instance.
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_SPI1, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // Register the ISR in the Interrupt Vector Table.
    IntRegister(SYS_INT_SPINT1, SPIIsr);
    IntChannelSet(SYS_INT_SPINT1, 2);
    IntSystemEnable(SYS_INT_SPINT1);

    // Reset.
    SPIReset(SOC_SPI_1_REGS);
    SPIOutOfReset(SOC_SPI_1_REGS);

    // Mode.
    uint32_t spipc0 = SPI_SPIPC0_SOMIFUN | SPI_SPIPC0_SIMOFUN | SPI_SPIPC0_CLKFUN | SPI_SPIPC0_ENAFUN;
    SPIModeConfigure(SOC_SPI_1_REGS, SPI_MASTER_MODE);
    SPIPinControl(SOC_SPI_1_REGS, 0, 0, (unsigned int *)&spipc0);

    // Config.
    SPIClkConfigure(SOC_SPI_1_REGS, SOC_SYSCLK_2_FREQ, 10000000, SPI_DATA_FORMAT0);
    SPICharLengthSet(SOC_SPI_1_REGS, 8, SPI_DATA_FORMAT0);
    SPIConfigClkFormat(SOC_SPI_1_REGS, SPI_CLK_POL_HIGH, SPI_DATA_FORMAT0);
    SPIDelayConfigure(SOC_SPI_1_REGS, 0, 0, 10, 10);
    SPIIntLevelSet(SOC_SPI_1_REGS, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);

    pbdrv_init_busy_up();
    process_start(&pbdrv_display_ev3_init_process);
}

/**
 * Display driver process. Initializes the display and updates the display
 * with the user frame buffer at a regular interval if the user data was
 * updated.
 */
PROCESS_THREAD(pbdrv_display_ev3_init_process, ev, data) {

    static struct etimer etimer;
    static uint32_t script_index;
    static uint8_t payload;

    PROCESS_BEGIN();

    #if ST7586S_DO_RESET_AND_INIT
    pbdrv_gpio_out_low(&pin_lcd_reset);
    etimer_set(&etimer, 10);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&etimer));
    pbdrv_gpio_out_high(&pin_lcd_reset);
    etimer_set(&etimer, 120);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&etimer));
    #endif // ST7586S_DO_RESET_AND_INIT

    // For every action in the init script, either send a command or data, or
    // wait for a given delay.
    for (script_index = 0; script_index < PBIO_ARRAY_SIZE(init_script); script_index++) {
        pbdrv_display_st7586s_action_t *action = &init_script[script_index];

        if (action->type == ST7586S_ACTION_DELAY) {
            // Simple delay.
            etimer_set(&etimer, action->payload);
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&etimer));
        } else {
            // Send command or data.
            payload = action->payload;
            if (action->type == ST7586S_ACTION_WRITE_DATA) {
                pbdrv_gpio_out_high(&pin_lcd_a0);
            } else {
                pbdrv_gpio_out_low(&pin_lcd_a0);
            }
            pbdrv_display_st7586s_write_data_begin(&payload, sizeof(payload));
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL && spi_status == SPI_STATUS_COMPLETE);
            pbdrv_gpio_out_high(&pin_lcd_cs);
        }
    }

    // Staying in data mode from here.
    pbdrv_gpio_out_high(&pin_lcd_a0);

    // Initial splash screen.
    pbdrv_display_load_indexed_bitmap(pbdrv_display_pybricks_logo);
    pbdrv_display_st7586s_encode_user_frame();
    pbdrv_display_st7586s_write_data_begin(st7586s_send_buf, sizeof(st7586s_send_buf));
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL && spi_status == SPI_STATUS_COMPLETE);
    pbdrv_gpio_out_high(&pin_lcd_cs);

    // Done initializing.
    pbdrv_init_busy_down();

    // Regularly update the display with the user frame buffer, if changed.
    etimer_set(&etimer, 40);
    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&etimer));
        if (pbdrv_display_user_frame_update_requested) {
            pbdrv_display_user_frame_update_requested = false;
            pbdrv_display_st7586s_encode_user_frame();
            pbdrv_display_st7586s_write_data_begin(st7586s_send_buf, sizeof(st7586s_send_buf));
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL && spi_status == SPI_STATUS_COMPLETE);
            pbdrv_gpio_out_high(&pin_lcd_cs);
        }
        etimer_reset(&etimer);
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_DISPLAY_EV3

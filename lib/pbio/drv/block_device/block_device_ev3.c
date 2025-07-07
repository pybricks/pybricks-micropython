// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Block device driver for N25Q128 SPI flash memory chip connected to TI AM1808.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_EV3

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../core.h"

#include <pbdrv/block_device.h>
#include <pbdrv/gpio.h>

#include <tiam1808/edma.h>
#include <tiam1808/spi.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/edma_event.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#include "block_device_ev3.h"
#include "../drv/gpio/gpio_ev3.h"

#include <pbio/error.h>
#include <pbio/int_math.h>

/**
 * SPI bus state.
 */
typedef enum {
    /** Operation complete, bus is idle. */
    SPI_STATUS_COMPLETE = 0,
    /** Operation failed. */
    SPI_STATUS_ERROR = 1,
} spi_status_t;

/**
 * The block device driver state.
 */
static struct {
    /** HAL Transfer status */
    volatile spi_status_t spi_status;
} bdev;


/**
 * Tx transfer complete.
 */
void pbdrv_block_device_ev3_spi_tx_complete(void) {
    // TODO
}

/**
 * Rx transfer complete.
 */
void pbdrv_block_device_ev3_spi_rx_complete(void) {
    // TODO
}

/**
 * Transfer error. // TODO: hook this up
 */
void pbdrv_block_device_ev3_spi_error(void) {
    bdev.spi_status = SPI_STATUS_ERROR;
    pbio_os_request_poll();
}

/**
 * SPI interrupt handler, which is only used for single-byte commands
 */
static void spi0_isr(void) {
    // TODO
}


// ADC / Flash SPI0 data MOSI
static const pbdrv_gpio_t pin_spi0_mosi = PBDRV_GPIO_EV3_PIN(3, 15, 12, 8, 5);
// ADC / Flash SPI0 data MISO
static const pbdrv_gpio_t pin_spi0_miso = PBDRV_GPIO_EV3_PIN(3, 11, 8, 8, 6);
// ADC / Flash SPI0 Clock
static const pbdrv_gpio_t pin_spi0_clk = PBDRV_GPIO_EV3_PIN(3, 3, 0, 1, 8);

// ADC SPI0 chip select (active low).
static const pbdrv_gpio_t pin_spi0_ncs3 = PBDRV_GPIO_EV3_PIN(3, 27, 24, 8, 2);
// Flash SPI0 chip select (active low).
static const pbdrv_gpio_t pin_spi0_ncs0 = PBDRV_GPIO_EV3_PIN(4, 7, 4, 1, 6);

// Flash write protect (active low).
static const pbdrv_gpio_t pin_flash_nwp = PBDRV_GPIO_EV3_PIN(12, 23, 20, 5, 2);
// Flash reset/hold (active low).
static const pbdrv_gpio_t pin_flash_nhold = PBDRV_GPIO_EV3_PIN(6, 31, 28, 2, 0);

/**
 * Bus speeds.
 */
enum {
    // The maximum allowed clock speed is /3 yielding 50 MHz
    // This happens to be below the speed where the FAST_READ command is required
    SPI_CLK_SPEED_FLASH = 50000000,
};

// -Hardware resource allocation notes-
//
// The SPI peripheral can be configured with multiple "data formats" which can be used for different peripherals.
// This controls things such as the clock speed, SPI CPOL/CPHA, and timing parameters.
// We use the following:
// - Format 0: Flash
// - Format 1: ADC (TODO)
//
// The EDMA3 peripheral has 128 parameter sets. 32 of them are triggered by events, but the others
// can be used by "linking" to them from a previous one. Instead of having an allocator for these,
// we hardcode the usage of linked slots (which means that we don't support arbitrary scatter-gather).
// - EDMA3_CHA_SPI0_TX: used to send initial command, links to 126 or 127
// - EDMA3_CHA_SPI0_RX: used to receive bytes corresponding to initial command, links to 125
// - 125: used to receive bytes corresponding to "user data"
// - 126: used to send all but the last byte, links to 127
// - 127: used to send the last byte, which is necessary to clear CSHOLD


static void set_address_be(uint8_t *buf, uint32_t address) {
    buf[0] = address >> 16;
    buf[1] = address >> 8;
    buf[2] = address;
}

/**
 * Device-specific flash commands.
 */
enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_GET_ID = 0x9F,
    FLASH_CMD_READ_DATA = 0x03,
    FLASH_CMD_ERASE_BLOCK = 0xD8,
    FLASH_CMD_WRITE_DATA = 0x02,
};

/**
 * Flash sizes.
 */
enum {
    FLASH_SIZE_ERASE = 64 * 1024,
    FLASH_SIZE_READ = UINT16_MAX, // Limited by DMA transfer size.
    FLASH_SIZE_WRITE = 256,
};

/**
 * Flash status flags.
 */
enum {
    FLASH_STATUS_BUSY = 0x01,
};

// N25Q128 manufacturer and device ID.
static const uint8_t device_id[] = {0x20, 0xba, 0x18};

pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size) {

    static uint32_t size_done;
    static uint32_t size_now;
    pbio_error_t err;

    (void)err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || offset + size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Split up reads to maximum chunk size.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_READ);

        // TODO: Actually implement reading
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_block_device_store(pbio_os_state_t *state, uint8_t *buffer, uint32_t size) {

    static uint32_t offset;
    static uint32_t size_now;
    static uint32_t size_done;
    pbio_error_t err;

    (void)err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Erase sector by sector.
    for (offset = 0; offset < size; offset += FLASH_SIZE_ERASE) {
        // TODO: Actually implement erasing
    }

    // Write page by page.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_WRITE);

        // TODO: Actually implement writing
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_os_process_t pbdrv_block_device_ev3_init_process;

pbio_error_t pbdrv_block_device_ev3_init_process_thread(pbio_os_state_t *state, void *context) {
    pbio_error_t err;

    (void)err;
    (void)device_id;

    PBIO_OS_ASYNC_BEGIN(state);

    // TODO: Get and verify flash ID

    // Verify flash device ID // REVISIT: Fix up id_data so we can memcmp
    // if (memcmp(device_id, id_data, sizeof(id_data))) {
    //     return PBIO_ERROR_FAILED;
    // }

    // Initialization done.
    pbdrv_init_busy_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_block_device_init(void) {
    // SPI module basic init
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_SPI0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    SPIReset(SOC_SPI_0_REGS);
    SPIOutOfReset(SOC_SPI_0_REGS);
    SPIModeConfigure(SOC_SPI_0_REGS, SPI_MASTER_MODE);
    unsigned int spipc0 = SPI_SPIPC0_SOMIFUN | SPI_SPIPC0_SIMOFUN | SPI_SPIPC0_CLKFUN | SPI_SPIPC0_SCS0FUN0 | SPI_SPIPC0_SCS0FUN3;
    SPIPinControl(SOC_SPI_0_REGS, 0, 0, &spipc0);
    SPIDefaultCSSet(SOC_SPI_0_REGS, (1 << PBDRV_EV3_SPI0_FLASH_CS) | (1 << PBDRV_EV3_SPI0_ADC_CS));

    // SPI module data formats
    SPIClkConfigure(SOC_SPI_0_REGS, SOC_SPI_0_MODULE_FREQ, SPI_CLK_SPEED_FLASH, SPI_DATA_FORMAT0);
    // For reasons which have not yet been fully investigated, attempting to switch between
    // SPI_CLK_POL_HIGH and SPI_CLK_POL_LOW seems to not work correctly (possibly causing a glitch?).
    // The suspected cause is that partial writes to SPI_SPIDAT1 do not update *anything*,
    // not even the clock idle state, until *after* the data field is written to.
    // Since multiple options work for SPI flash but the ADC requires one particular setting,
    // set this CPOL/CPHA to match what the ADC needs.
    SPIConfigClkFormat(SOC_SPI_0_REGS, SPI_CLK_POL_LOW | SPI_CLK_OUTOFPHASE, SPI_DATA_FORMAT0);
    SPIShiftMsbFirst(SOC_SPI_0_REGS, SPI_DATA_FORMAT0);
    SPICharLengthSet(SOC_SPI_0_REGS, 8, SPI_DATA_FORMAT0);
    // TODO: Initialize ADC data format

    // Configure the GPIO pins.
    pbdrv_gpio_alt(&pin_spi0_mosi, SYSCFG_PINMUX3_PINMUX3_15_12_SPI0_SIMO0);
    pbdrv_gpio_alt(&pin_spi0_miso, SYSCFG_PINMUX3_PINMUX3_11_8_SPI0_SOMI0);
    pbdrv_gpio_alt(&pin_spi0_clk, SYSCFG_PINMUX3_PINMUX3_3_0_SPI0_CLK);
    pbdrv_gpio_alt(&pin_spi0_ncs0, SYSCFG_PINMUX4_PINMUX4_7_4_NSPI0_SCS0);
    pbdrv_gpio_alt(&pin_spi0_ncs3, SYSCFG_PINMUX3_PINMUX3_27_24_NSPI0_SCS3);

    // Configure the flash control pins and put them with the values we want
    pbdrv_gpio_alt(&pin_flash_nwp, SYSCFG_PINMUX12_PINMUX12_23_20_GPIO5_2);
    pbdrv_gpio_out_high(&pin_flash_nwp);
    pbdrv_gpio_alt(&pin_flash_nhold, SYSCFG_PINMUX6_PINMUX6_31_28_GPIO2_0);
    pbdrv_gpio_out_high(&pin_flash_nhold);

    // Set up interrupts
    SPIIntLevelSet(SOC_SPI_0_REGS, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);
    IntRegister(SYS_INT_SPINT0, spi0_isr);
    IntChannelSet(SYS_INT_SPINT0, 2);
    IntSystemEnable(SYS_INT_SPINT0);

    // Request DMA channels. This only needs to be done for the initial events (and not for linked parameter sets)
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, EDMA3_CHA_SPI0_TX, EDMA3_CHA_SPI0_TX, 0);
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, EDMA3_CHA_SPI1_RX, EDMA3_CHA_SPI1_RX, 0);

    // Enable!
    SPIEnable(SOC_SPI_0_REGS);

    (void)set_address_be;

    bdev.spi_status = SPI_STATUS_COMPLETE;

    pbdrv_init_busy_up();
    pbio_os_process_start(&pbdrv_block_device_ev3_init_process, pbdrv_block_device_ev3_init_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_EV3

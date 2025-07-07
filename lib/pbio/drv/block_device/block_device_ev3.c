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
#include "../drv/adc/adc_ev3.h"

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
    /** Waiting for TX to complete. Bitfield. */
    SPI_STATUS_WAIT_TX = 0x100,
    /** Waiting for RX to complete. Bitfield. */
    SPI_STATUS_WAIT_RX = 0x200,
    /** Waiting for anything to complete. Bit mask. */
    SPI_STATUS_WAIT_ANY = 0x300,
} spi_status_t;

/**
 * Driver-specific sizes.
 */
enum {
    // This is large enough for all flash commands we use
    SPI_CMD_BUF_SZ = 4,
    // Limited by DMA descriptor
    SPI_MAX_DATA_SZ = 0xffff,
};

/**
 * The block device driver state.
 */
static struct {
    /** HAL Transfer status */
    volatile spi_status_t spi_status;

    // This is used when SPI only needs to receive. It should always stay as 0.
    uint8_t tx_dummy_byte;
    // This is used when received data is to be discarded. Its value should be ignored.
    uint8_t rx_dummy_byte;
    // This is used when transmitting so that the last byte clears CSHOLD.
    uint32_t tx_last_word;
    // This is used to hold the initial command to the SPI peripheral.
    uint8_t spi_cmd_buf_tx[SPI_CMD_BUF_SZ];
    // This is used to hold the replies to commands to the SPI peripheral.
    uint8_t spi_cmd_buf_rx[SPI_CMD_BUF_SZ];
} bdev;


/**
 * Tx transfer complete.
 */
void pbdrv_block_device_ev3_spi_tx_complete(void) {
    bdev.spi_status &= ~SPI_STATUS_WAIT_TX;
    if (!(bdev.spi_status & SPI_STATUS_WAIT_ANY)) {
        SPIIntDisable(SOC_SPI_0_REGS, SPI_DMA_REQUEST_ENA_INT);
        pbio_os_request_poll();
    }
}

/**
 * Rx transfer complete.
 */
void pbdrv_block_device_ev3_spi_rx_complete(void) {
    bdev.spi_status &= ~SPI_STATUS_WAIT_RX;
    if (!(bdev.spi_status & SPI_STATUS_WAIT_ANY)) {
        SPIIntDisable(SOC_SPI_0_REGS, SPI_DMA_REQUEST_ENA_INT);
        pbio_os_request_poll();
    }
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
    uint32_t intCode = 0;
    IntSystemStatusClear(SYS_INT_SPINT0);

    while ((intCode = SPIInterruptVectorGet(SOC_SPI_0_REGS))) {
        if (intCode != SPI_RECV_FULL) {
            continue;
        }

        bdev.spi_cmd_buf_rx[0] = HWREG(SOC_SPI_0_REGS + SPI_SPIBUF);
        bdev.spi_status &= ~SPI_STATUS_WAIT_RX;
        SPIIntDisable(SOC_SPI_0_REGS, SPI_RECV_INT);
        pbio_os_request_poll();
    }
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
// - Format 1: ADC
//
// The EDMA3 peripheral has 128 parameter sets. 32 of them are triggered by events, but the others
// can be used by "linking" to them from a previous one. Instead of having an allocator for these,
// we hardcode the usage of linked slots (which means that we don't support arbitrary scatter-gather).
// - EDMA3_CHA_SPI0_TX: used to send initial command, links to 126 or 127
// - EDMA3_CHA_SPI0_RX: used to receive bytes corresponding to initial command, links to 125
// - 125: used to receive bytes corresponding to "user data"
// - 126: used to send all but the last byte, links to 127
// - 127: used to send the last byte, which is necessary to clear CSHOLD


// XXX In the TI StarterWare code, miscompiles seemed to be happening due to strict aliasing issues with this type.
// Fix it by using a union for type punning, which is more-or-less allowed
// (it is explicitly allowed by GCC, and we do not have trap representations on this platform)
//
// This declaration also forces alignment
typedef union {
    EDMA3CCPaRAMEntry p;
    uint32_t u[32 / 4];
} EDMA3CCPaRAMEntry_;

static void edma3_set_param(unsigned int slot, EDMA3CCPaRAMEntry_ *p) {
    for (int i = 0; i < 32 / 4; i++) {
        HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_PaRAM_BASE + slot * 32 + i * 4) = p->u[i];
    }
}


// Helper functions for setting up the high control bits of a data transfer
static inline void spi0_setup_for_flash() {
    *(volatile uint16_t *)(SOC_SPI_0_REGS + SPI_SPIDAT1 + 2) =
        (1 << (SPI_SPIDAT1_CSHOLD_SHIFT - 16)) |
        (SPI_SPIDAT1_DFSEL_FORMAT0 << (SPI_SPIDAT1_DFSEL_SHIFT - 16)) |
        (1 << PBDRV_EV3_SPI0_ADC_CS) | (0 << PBDRV_EV3_SPI0_FLASH_CS);      // Deselect ADC, select flash
}
static inline uint32_t spi0_last_dat1_for_flash(uint8_t x) {
    return x |
           (SPI_SPIDAT1_DFSEL_FORMAT0 << SPI_SPIDAT1_DFSEL_SHIFT) |
           (1 << (SPI_SPIDAT1_CSNR_SHIFT + PBDRV_EV3_SPI0_ADC_CS)) |
           (0 << (SPI_SPIDAT1_CSNR_SHIFT + PBDRV_EV3_SPI0_FLASH_CS));   // Deselect ADC, select flash
}

/**
 * Initiates an SPI transfer via DMA, specifically designed for SPI flash command styles.
 *
 * @param [in] cmd              Bytes to initially transfer as the command for the flash
 * @param [in] cmd_len          Length of \p cmd
 * @param [in] user_data_tx     Bytes to be sent to the flash after the initial command. May be null, in which case a dummy value is sent.
 *                              Lifetime must remain valid until after the completion of the entire transfer.
 * @param [out] user_data_rx    Bytes to be received from the flash after the initial command. May be null.
 *                              Lifetime must remain valid until after the completion of the entire transfer.
 * @param [in] user_data_len    Length of the user data (both transmission and reception)
 * @return                      ::PBIO_SUCCESS on success.
 *                              ::PBIO_ERROR_BUSY if SPI is busy.
 *                              ::PBIO_ERROR_INVALID_ARG if argument is too big
 *                              ::PBIO_ERROR_IO for other errors.
 */
static pbio_error_t spi_begin_for_flash(
    const unsigned char *cmd,
    unsigned int cmd_len,
    const unsigned char *user_data_tx,
    unsigned char *user_data_rx,
    unsigned int user_data_len
    ) {
    EDMA3CCPaRAMEntry_ ps;

    if (cmd_len > SPI_CMD_BUF_SZ || user_data_len > SPI_MAX_DATA_SZ) {
        // Maximum size exceeded
        return PBIO_ERROR_INVALID_ARG;
    }
    if (bdev.spi_status & SPI_STATUS_WAIT_ANY) {
        // Another read operation is already in progress.
        return PBIO_ERROR_BUSY;
    }
    if (bdev.spi_status == SPI_STATUS_ERROR) {
        // Previous transmission went wrong.
        return PBIO_ERROR_IO;
    }

    spi0_setup_for_flash();

    if (cmd_len == 1 && user_data_len == 0) {
        // There is no point in using DMA here
        // (this is used for e.g. WREN)

        bdev.spi_status = SPI_STATUS_WAIT_RX;

        uint32_t tx = spi0_last_dat1_for_flash(cmd[0]);
        SPIIntEnable(SOC_SPI_0_REGS, SPI_RECV_INT);
        HWREG(SOC_SPI_0_REGS + SPI_SPIDAT1) = tx;
    } else {
        memcpy(&bdev.spi_cmd_buf_tx, cmd, cmd_len);

        if (user_data_len == 0) {
            // Only a command, no user data

            bdev.tx_last_word = spi0_last_dat1_for_flash(cmd[cmd_len - 1]);

            // TX everything except last byte
            ps.p.srcAddr = (unsigned int)(&bdev.spi_cmd_buf_tx);
            ps.p.destAddr = SOC_SPI_0_REGS + SPI_SPIDAT1;
            ps.p.aCnt = 1;
            ps.p.bCnt = cmd_len - 1;
            ps.p.cCnt = 1;
            ps.p.srcBIdx = 1;
            ps.p.destBIdx = 0;
            ps.p.srcCIdx = 0;
            ps.p.destCIdx = 0;
            ps.p.linkAddr = 127 * 32;
            ps.p.bCntReload = 0;
            ps.p.opt = 0;
            edma3_set_param(EDMA3_CHA_SPI0_TX, &ps);

            // TX last byte, clearing CSHOLD
            ps.p.srcAddr = (unsigned int)(&bdev.tx_last_word);
            ps.p.aCnt = 4;
            ps.p.bCnt = 1;
            ps.p.linkAddr = 0xffff;
            ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_TX << EDMA3CC_OPT_TCC_SHIFT);
            edma3_set_param(127, &ps);

            // RX all bytes
            ps.p.srcAddr = SOC_SPI_0_REGS + SPI_SPIBUF;
            ps.p.destAddr = (unsigned int)(&bdev.spi_cmd_buf_rx);
            ps.p.aCnt = 1;
            ps.p.bCnt = cmd_len;
            ps.p.cCnt = 1;
            ps.p.srcBIdx = 0;
            ps.p.destBIdx = 1;
            ps.p.srcCIdx = 0;
            ps.p.destCIdx = 0;
            ps.p.linkAddr = 0xffff;
            ps.p.bCntReload = 0;
            ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_RX << EDMA3CC_OPT_TCC_SHIFT);
            edma3_set_param(EDMA3_CHA_SPI0_RX, &ps);
        } else {
            // Command *and* user data

            // TX the command
            ps.p.srcAddr = (unsigned int)(&bdev.spi_cmd_buf_tx);
            ps.p.destAddr = SOC_SPI_0_REGS + SPI_SPIDAT1;
            ps.p.aCnt = 1;
            ps.p.bCnt = cmd_len;
            ps.p.cCnt = 1;
            ps.p.srcBIdx = 1;
            ps.p.destBIdx = 0;
            ps.p.srcCIdx = 0;
            ps.p.destCIdx = 0;
            ps.p.linkAddr = 126 * 32;
            ps.p.bCntReload = 0;
            ps.p.opt = 0;
            edma3_set_param(EDMA3_CHA_SPI0_TX, &ps);

            if (user_data_tx) {
                bdev.tx_last_word = spi0_last_dat1_for_flash(user_data_tx[user_data_len - 1]);

                // TX all but the last byte
                ps.p.srcAddr = (unsigned int)(user_data_tx);
                ps.p.bCnt = user_data_len - 1;
                ps.p.linkAddr = 127 * 32;
                edma3_set_param(126, &ps);

                // TX the last byte, clearing CSHOLD
                ps.p.srcAddr = (unsigned int)(&bdev.tx_last_word);
                ps.p.aCnt = 4;
                ps.p.bCnt = 1;
                ps.p.linkAddr = 0xffff;
                ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_TX << EDMA3CC_OPT_TCC_SHIFT);
                edma3_set_param(127, &ps);
            } else {
                bdev.tx_last_word = spi0_last_dat1_for_flash(0);

                // TX all but the last byte
                ps.p.srcAddr = (unsigned int)(&bdev.tx_dummy_byte);
                ps.p.bCnt = user_data_len - 1;
                ps.p.srcBIdx = 0;
                ps.p.linkAddr = 127 * 32;
                edma3_set_param(126, &ps);

                // TX the last byte, clearing CSHOLD
                ps.p.srcAddr = (unsigned int)(&bdev.tx_last_word);
                ps.p.aCnt = 4;
                ps.p.bCnt = 1;
                ps.p.linkAddr = 0xffff;
                ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_TX << EDMA3CC_OPT_TCC_SHIFT);
                edma3_set_param(127, &ps);
            }

            // RX the command
            ps.p.srcAddr = SOC_SPI_0_REGS + SPI_SPIBUF;
            ps.p.destAddr = (unsigned int)(&bdev.spi_cmd_buf_rx);
            ps.p.aCnt = 1;
            ps.p.bCnt = cmd_len;
            ps.p.cCnt = 1;
            ps.p.srcBIdx = 0;
            ps.p.destBIdx = 1;
            ps.p.srcCIdx = 0;
            ps.p.destCIdx = 0;
            ps.p.linkAddr = 125 * 32;
            ps.p.bCntReload = 0;
            ps.p.opt = 0;
            edma3_set_param(EDMA3_CHA_SPI0_RX, &ps);

            if (user_data_rx) {
                // RX the data
                ps.p.destAddr = (unsigned int)(user_data_rx);
                ps.p.bCnt = user_data_len;
                ps.p.linkAddr = 0xffff;
                ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_RX << EDMA3CC_OPT_TCC_SHIFT);
                edma3_set_param(125, &ps);
            } else {
                // RX dummy
                ps.p.destAddr = (unsigned int)(&bdev.rx_dummy_byte);
                ps.p.bCnt = user_data_len;
                ps.p.destBIdx = 0;
                ps.p.linkAddr = 0xffff;
                ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_RX << EDMA3CC_OPT_TCC_SHIFT);
                edma3_set_param(125, &ps);
            }
        }

        bdev.spi_status = SPI_STATUS_WAIT_TX | SPI_STATUS_WAIT_RX;

        // TODO: pbio probably needs a framework for memory barriers and DMA cache management
        __asm__ volatile ("" ::: "memory");

        EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI0_TX, EDMA3_TRIG_MODE_EVENT);
        EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI0_RX, EDMA3_TRIG_MODE_EVENT);
        SPIIntEnable(SOC_SPI_0_REGS, SPI_DMA_REQUEST_ENA_INT);
    }

    return PBIO_SUCCESS;
}

/**
 * Initiates an SPI transfer via DMA, specifically designed for the ADC.
 *
 * @param [in] cmds             Data (both ADC chip commands and SPI peripheral commands) to be sent.
 *                              Lifetime must remain valid until after the completion of the entire transfer.
 * @param [in] data             Buffer for ADC chip outputs.
 *                              Lifetime must remain valid until after the completion of the entire transfer.
 * @param [in] len              Length of \p cmds and \p data
 * @return                      ::PBIO_SUCCESS on success.
 *                              ::PBIO_ERROR_BUSY if SPI is busy.
 *                              ::PBIO_ERROR_INVALID_ARG if argument is too big
 *                              ::PBIO_ERROR_IO for other errors.
 */
pbio_error_t pbdrv_block_device_ev3_spi_begin_for_adc(const uint32_t *cmds, volatile uint16_t *data, unsigned int len) {
    EDMA3CCPaRAMEntry_ ps;

    if (len > SPI_MAX_DATA_SZ) {
        // Maximum size exceeded
        return PBIO_ERROR_INVALID_ARG;
    }
    if (bdev.spi_status & SPI_STATUS_WAIT_ANY) {
        // Another read operation is already in progress.
        return PBIO_ERROR_BUSY;
    }
    if (bdev.spi_status == SPI_STATUS_ERROR) {
        // Previous transmission went wrong.
        return PBIO_ERROR_IO;
    }

    ps.p.srcAddr = (unsigned int)(cmds);
    ps.p.destAddr = SOC_SPI_0_REGS + SPI_SPIDAT1;
    ps.p.aCnt = sizeof(uint32_t);
    ps.p.bCnt = len;
    ps.p.cCnt = 1;
    ps.p.srcBIdx = sizeof(uint32_t);
    ps.p.destBIdx = 0;
    ps.p.srcCIdx = 0;
    ps.p.destCIdx = 0;
    ps.p.linkAddr = 0xffff;
    ps.p.bCntReload = 0;
    ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_TX << EDMA3CC_OPT_TCC_SHIFT);
    edma3_set_param(EDMA3_CHA_SPI0_TX, &ps);

    ps.p.srcAddr = SOC_SPI_0_REGS + SPI_SPIBUF;
    ps.p.destAddr = (unsigned int)(data);
    ps.p.aCnt = sizeof(uint16_t);
    ps.p.bCnt = len;
    ps.p.srcBIdx = 0;
    ps.p.destBIdx = sizeof(uint16_t);
    ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_RX << EDMA3CC_OPT_TCC_SHIFT);
    edma3_set_param(EDMA3_CHA_SPI0_RX, &ps);

    bdev.spi_status = SPI_STATUS_WAIT_TX | SPI_STATUS_WAIT_RX;

    // TODO: pbio probably needs a framework for memory barriers and DMA cache management
    __asm__ volatile ("" ::: "memory");

    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI0_TX, EDMA3_TRIG_MODE_EVENT);
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI0_RX, EDMA3_TRIG_MODE_EVENT);
    SPIIntEnable(SOC_SPI_0_REGS, SPI_DMA_REQUEST_ENA_INT);

    return PBIO_SUCCESS;
}


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
    FLASH_SIZE_READ = SPI_MAX_DATA_SZ,
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

// Request flash device ID.
static const uint8_t cmd_rdid[] = {FLASH_CMD_GET_ID, 0x00, 0x00, 0x00};

// Request the write-status byte.
static const uint8_t cmd_status[] = {FLASH_CMD_GET_STATUS, 0x00};

// Enable flash writing. Needed once before each write operation.
static const uint8_t cmd_write_enable[] = {FLASH_CMD_WRITE_ENABLE};

// Request reading from address. Buffer: read command + address + dummy byte.
// Should be followed by another command that reads the data.
static uint8_t read_address[4] = {FLASH_CMD_READ_DATA};

// Request page write at address. Buffer: write command + address.
// Should be followed by the data.
static uint8_t write_address[4] = {FLASH_CMD_WRITE_DATA};

// Request sector erase at address. Buffer: erase command + address.
static uint8_t erase_address[4] = {FLASH_CMD_ERASE_BLOCK};

pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size) {

    static uint32_t size_done;
    static uint32_t size_now;
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || offset + size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Split up reads to maximum chunk size.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_READ);

        // Set address for this read request and send it.
        set_address_be(&read_address[1], PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + offset + size_done);
        err = spi_begin_for_flash(read_address, sizeof(read_address), 0, buffer + size_done, size_now);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        PBIO_OS_AWAIT_WHILE(state, bdev.spi_status & SPI_STATUS_WAIT_ANY);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Poll the status register waiting for writes to complete.
 */
static pbio_error_t flash_wait_write(pbio_os_state_t *state) {
    uint8_t status;
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    do {
        err = spi_begin_for_flash(cmd_status, sizeof(cmd_status), 0, 0, 0);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        PBIO_OS_AWAIT_WHILE(state, bdev.spi_status & SPI_STATUS_WAIT_ANY);

        status = bdev.spi_cmd_buf_rx[1];
    } while (status & FLASH_STATUS_BUSY);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_block_device_store(pbio_os_state_t *state, uint8_t *buffer, uint32_t size) {

    static pbio_os_state_t sub;
    static uint32_t offset;
    static uint32_t size_now;
    static uint32_t size_done;
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    #if PBDRV_CONFIG_ADC_EV3
    // HACK
    // We only store on shutdown. Block ADC.
    pbdrv_adc_ev3_shut_down_hack();
    PBIO_OS_AWAIT_UNTIL(state, pbdrv_adc_ev3_is_shut_down_hack());
    #endif

    // Erase sector by sector.
    for (offset = 0; offset < size; offset += FLASH_SIZE_ERASE) {
        // Enable writing
        err = spi_begin_for_flash(cmd_write_enable, sizeof(cmd_write_enable), 0, 0, 0);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        PBIO_OS_AWAIT_WHILE(state, bdev.spi_status & SPI_STATUS_WAIT_ANY);

        // Erase this block
        set_address_be(&erase_address[1], PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + offset);
        err = spi_begin_for_flash(erase_address, sizeof(erase_address), 0, 0, 0);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        PBIO_OS_AWAIT_WHILE(state, bdev.spi_status & SPI_STATUS_WAIT_ANY);

        // Wait for completion
        PBIO_OS_AWAIT(state, &sub, err = flash_wait_write(&sub));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Write page by page.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_WRITE);

        // Enable writing
        err = spi_begin_for_flash(cmd_write_enable, sizeof(cmd_write_enable), 0, 0, 0);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        PBIO_OS_AWAIT_WHILE(state, bdev.spi_status & SPI_STATUS_WAIT_ANY);

        // Write this block
        set_address_be(&write_address[1], PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + size_done);
        err = spi_begin_for_flash(write_address, sizeof(write_address), buffer + size_done, 0, size_now);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        PBIO_OS_AWAIT_WHILE(state, bdev.spi_status & SPI_STATUS_WAIT_ANY);

        // Wait for completion
        PBIO_OS_AWAIT(state, &sub, err = flash_wait_write(&sub));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_os_process_t pbdrv_block_device_ev3_init_process;

pbio_error_t pbdrv_block_device_ev3_init_process_thread(pbio_os_state_t *state, void *context) {
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Write the ID getter command
    err = spi_begin_for_flash(cmd_rdid, sizeof(cmd_rdid), 0, 0, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT_WHILE(state, bdev.spi_status & SPI_STATUS_WAIT_ANY);

    // Verify flash device ID
    if (memcmp(device_id, &bdev.spi_cmd_buf_rx[1], sizeof(device_id))) {
        return PBIO_ERROR_FAILED;
    }

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

    bdev.spi_status = SPI_STATUS_COMPLETE;

    pbdrv_init_busy_up();
    pbio_os_process_start(&pbdrv_block_device_ev3_init_process, pbdrv_block_device_ev3_init_process_thread, NULL);
}

int pbdrv_block_device_ev3_is_busy() {
    return bdev.spi_status & SPI_STATUS_WAIT_ANY;
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_EV3

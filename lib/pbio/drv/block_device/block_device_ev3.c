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

#include <pbdrv/../../drv/uart/uart_debug_first_port.h>

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
 * Whether to receive (read) or send (write).
 */
typedef enum {
    /** Receive data from SPI device. */
    SPI_RECV = 0x00,
    /** Send data to SPI device. */
    SPI_SEND = 0x01,
    /** Bitflag to keep NCS low after the operation. Only used with SPI_SEND. */
    SPI_CS_KEEP_ENABLED = 0x02,
} spi_operation_t;

/**
 * SPI command to send or receive data with a buffer of a given size.
 */
typedef struct {
    /** Whether to read or write, and whether to keep CS enabled when done. */
    spi_operation_t operation;
    /** Buffer to write from or read into. */
    uint8_t *buffer;
    /** Buffer size. */
    uint16_t size;
} spi_command_t;

#define SPI_CMD_BUF_SZ      4

/**
 * The block device driver state.
 */
static struct {
    /** HAL Transfer status */
    volatile spi_status_t spi_status;

    // This is programmed into the DMA controller when SPI only needs to receive.
    // It should always stay as 0.
    uint8_t tx_dummy_byte;
    // This is used when transmitting user data, so that the last byte clears CSHOLD.
    uint32_t tx_last_word;
    // This is programmed into the DMA controller when SPI only needs to receive.
    uint8_t rx_dummy_byte;
    // This is used to hold the initial command to the SPI peripheral.
    uint8_t spi_cmd_buf[SPI_CMD_BUF_SZ];
} bdev;


/**
 * Tx transfer complete.
 */
void pbdrv_block_device_ev3_spi_tx_complete(void) {
    bdev.spi_status &= ~SPI_STATUS_WAIT_TX;
    if (!(bdev.spi_status & SPI_STATUS_WAIT_ANY))
        pbio_os_request_poll();
}

/**
 * Rx transfer complete.
 */
void pbdrv_block_device_ev3_spi_rx_complete(void) {
    bdev.spi_status &= ~SPI_STATUS_WAIT_RX;
    if (!(bdev.spi_status & SPI_STATUS_WAIT_ANY))
        pbio_os_request_poll();
}

/**
 * Transfer error. // Leaving this here for inspiration as hook for TI API. Delete if not needed.
 */
void pbdrv_block_device_ev3_spi_error(void) {
    bdev.spi_status = SPI_STATUS_ERROR;
    pbio_os_request_poll();
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

// The maximum allowed clock speed is /3 yielding 50 MHz
// This happens to be below the speed where the FAST_READ command is required
#define SPI_CLK_SPEED_FLASH     50000000

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
// - EDMA3_CHA_SPI0_TX: used to send initial command, chains to 126
// - EDMA3_CHA_SPI0_RX: used to receive bytes corresponding to initial command, chains to 125
// - 125: used to receive bytes corresponding to "user data"
// - 126: used to send all but the last byte of the "user data" block, chains to 127
// - 127: used to send the last byte of the "user data" block, which is necessary to clear CSHOLD

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
    for (int i = 0; i < 32 / 4; i++)
        HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_PaRAM_BASE + slot*32 + i*4) = p->u[i];
}

// /**
//  * Sets or clears the chip select line.
//  *
//  * // REVISIT: TI API also seems to have ways of having the peripheral do this for us. Whatever works is fine.
//  *
//  * @param [in] set         Whether to set (true) or clear (false) CS.
//  */
// static void spi_chip_select(bool set) {
//     // Active low, so set CS means set /CS low.
//     if (set) {
//         pbdrv_gpio_out_low(&pin_spi0_ncs0);
//     } else {
//         pbdrv_gpio_out_high(&pin_spi0_ncs0);
//     }
// }

static inline void spi0_setup_for_flash() {
    *(volatile uint16_t *)(SOC_SPI_0_REGS + SPI_SPIDAT1 + 2) =
        (1 << (SPI_SPIDAT1_CSHOLD_SHIFT - 16)) |
        (SPI_SPIDAT1_DFSEL_FORMAT0 << (SPI_SPIDAT1_DFSEL_SHIFT - 16)) |
        (1 << 3) | (0 << 0);    // nCS3 inactive, nCS0 active
}
static inline uint32_t spi0_last_dat1_for_flash(uint8_t x) {
    return x |
        (SPI_SPIDAT1_DFSEL_FORMAT0 << SPI_SPIDAT1_DFSEL_SHIFT) |
        (1 << (SPI_SPIDAT1_CSNR_SHIFT + 3)) | (0 << (SPI_SPIDAT1_CSNR_SHIFT + 0));  // nCS3 inactive, nCS0 active
}

// /**
//  * Initiates an SPI transfer via DMA.
//  *
//  * @param [in] cmd         The command to start.
//  * @return                 ::PBIO_SUCCESS on success.
//  *                         ::PBIO_ERROR_BUSY if SPI is busy.
//  *                         ::PBIO_ERROR_INVALID_ARG for invalid args to HAL call.
//  *                         ::PBIO_ERROR_IO for other errors.
//  */
// static pbio_error_t spi_begin(const spi_command_t *cmd) {

//     if (bdev.spi_status == SPI_STATUS_WAIT) {
//         // Another read operation is already in progress.
//         return PBIO_ERROR_BUSY;
//     }
//     if (bdev.spi_status == SPI_STATUS_ERROR) {
//         // Previous transmission went wrong.
//         return PBIO_ERROR_IO;
//     }
//     // Set status to wait and start receiving.
//     bdev.spi_status = SPI_STATUS_WAIT;

//     // REPLACE WITH TI AM1808 SPI DMA operation
//     //
//     //
//     // // Start SPI operation.
//     // HAL_StatusTypeDef err;
//     // if (cmd->operation == SPI_RECV) {
//     //     err = HAL_SPI_Receive_DMA(&bdev.hspi, cmd->buffer, cmd->size);
//     // } else {
//     //     err = HAL_SPI_Transmit_DMA(&bdev.hspi, cmd->buffer, cmd->size);
//     // }

//     // // Handle HAL errors.
//     // switch (err) {
//     //     case HAL_OK:
//     //         return PBIO_SUCCESS;
//     //     case HAL_ERROR:
//     //         return PBIO_ERROR_INVALID_ARG;
//     //     case HAL_BUSY:
//     //         return PBIO_ERROR_BUSY;
//     //     default:
//     //         return PBIO_ERROR_IO;
//     // }

//     // REVISIT: DELETE ME. Here for now until we implement a real transfer.
//     bdev.spi_status = SPI_STATUS_COMPLETE;
//     return PBIO_SUCCESS;
// }

// /**
//  * Starts and awaits an SPI transfer.
//  */
// static pbio_error_t spi_command_thread(pbio_os_state_t *state, const spi_command_t *cmd) {

//     pbio_error_t err;

//     PBIO_OS_ASYNC_BEGIN(state);

//     // Select peripheral.
//     spi_chip_select(true);

//     // Start SPI operation.
//     err = spi_begin(cmd);
//     if (err != PBIO_SUCCESS) {
//         spi_chip_select(false);
//         return err;
//     }

//     // Wait until SPI operation completes.
//     PBIO_OS_AWAIT_UNTIL(state, bdev.spi_status == SPI_STATUS_COMPLETE);

//     // Turn off peripheral if requested.
//     if (!(cmd->operation & SPI_CS_KEEP_ENABLED)) {
//         spi_chip_select(false);
//     }

//     PBIO_OS_ASYNC_END(PBIO_SUCCESS);
// }

//
// REVISIT: Can delete this, and hardcode for N25Q128
//
// Select constant values based on flash device type.
#if 1
#define W25Qxx(Q32, Q256) (Q32)
#else
#define W25Qxx(Q32, Q256) (Q256)
#endif

// 3 byte addressing mode.
#define FLASH_ADDRESS_SIZE (3)

// static void set_address_be(uint8_t *buf, uint32_t address) {
//     buf[0] = address >> 16;
//     buf[1] = address >> 8;
//     buf[2] = address;
// }

/**
 * Device-specific flash commands. // REVISIT: Hardcode for N25Q128
 */
enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_GET_ID = 0x9F,
    FLASH_CMD_READ_DATA = W25Qxx(0x0B, 0x0C),
    FLASH_CMD_ERASE_BLOCK = W25Qxx(0x20, 0x21),
    FLASH_CMD_WRITE_DATA = W25Qxx(0x02, 0x12),
};

/**
 * Flash sizes. // REVISIT: for N25Q128
 */
enum {
    FLASH_SIZE_ERASE = 4 * 1024, // Limited by W25QXX operation
    FLASH_SIZE_READ = UINT16_MAX, // Limited by STM32 DMA transfer size.
    FLASH_SIZE_WRITE = 256, // Limited by W25QXX operation
};

/**
 * Flash status flags.  // REVISIT: for N25Q128
 */
enum {
    FLASH_STATUS_BUSY = 0x01,
    FLASH_STATUS_WRITE_ENABLED = 0x02,
};

// // W25Qxx manufacturer and device ID.  // REVISIT: for N25Q128
// static const uint8_t device_id[] = {0xEF, 0x40, W25Qxx(0x16, 0x19)};

// // Request flash device ID.
// static const spi_command_t cmd_id_tx = {
//     .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
//     .buffer = &(uint8_t) {FLASH_CMD_GET_ID},
//     .size = 1,
// };

// // Receive flash device ID after sending request.
// static uint8_t id_data[sizeof(device_id)];
// static const spi_command_t cmd_id_rx = {
//     .operation = SPI_RECV,
//     .buffer = id_data,
//     .size = sizeof(id_data),
// };

// // Enable flash writing.
// static const spi_command_t cmd_write_enable = {
//     .operation = SPI_SEND,
//     .buffer = &(uint8_t) {FLASH_CMD_WRITE_ENABLE},
//     .size = 1,
// };

// // Request the write-status byte.
// static const spi_command_t cmd_status_tx = {
//     .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
//     .buffer = &(uint8_t) {FLASH_CMD_GET_STATUS},
//     .size = 1,
// };

// // Get the write-status byte.
// static uint8_t status;
// static const spi_command_t cmd_status_rx = {
//     .operation = SPI_RECV,
//     .buffer = (uint8_t *)&status,
//     .size = sizeof(status),
// };

// // Request reading from address. Buffer: read command + address + dummy byte.
// // Should be followed by another command that reads the data.
// static uint8_t read_address[1 + FLASH_ADDRESS_SIZE + 1] = {FLASH_CMD_READ_DATA};
// static const spi_command_t cmd_request_read = {
//     .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
//     .buffer = read_address,
//     .size = sizeof(read_address),
// };

// // Request page write at address. Buffer: write command + address.
// // Should be followed by cmd_data_read to read the data.
// static uint8_t write_address[1 + FLASH_ADDRESS_SIZE] = {FLASH_CMD_WRITE_DATA};
// static const spi_command_t cmd_request_write = {
//     .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
//     .buffer = write_address,
//     .size = sizeof(write_address),
// };

// // Request sector erase at address. Buffer: erase command + address.
// static uint8_t erase_address[1 + FLASH_ADDRESS_SIZE] = {FLASH_CMD_ERASE_BLOCK};
// static const spi_command_t cmd_request_erase = {
//     .operation = SPI_SEND,
//     .buffer = erase_address,
//     .size = sizeof(erase_address),
// };

// // Transmit data. Buffer and size should be set wherever this is used.
// static spi_command_t cmd_data_write = {
//     .operation = SPI_SEND,
// };

// // Read data. Buffer and size should be set wherever this is used.
// static spi_command_t cmd_data_read = {
//     .operation = SPI_RECV,
// };

// REVISIT: We are not supposed to include these. These are here just to have
// some placeholder data to play with until read/write are implemented
#include "genhdr/mpversion.h"
#include <pbsys/storage.h>

static pbio_error_t delete_me_load_placeholder_data(uint32_t offset, uint8_t *buffer, uint32_t size) {

    // This is a compiled version of the following. It:
    // - blinks the LED
    // - increments a persistent counter each time you run it
    //
    // For now, it increases the count each time you press the center button.
    // For now, it resets to 0 on shutdown and restart. If read and write are
    // implemented, the value is persistent!
    /*

    from pybricks.hubs import EV3Brick
    from pybricks.parameters import Color
    from pybricks.tools import wait

    hub = EV3Brick()

    count = hub.system.storage(0, read=1)[0]
    hub.system.storage(0, write=bytes([(count + 1) % 256]))

    print("Persistent count:", count)

    while True:
        hub.light.on(Color.RED)
        wait(500)
        hub.light.on(Color.GREEN)
        wait(500)
    */


    static const uint8_t _program_data[] = {
        0x43, 0x01, 0x00, 0x00, 0x5F, 0x5F, 0x6D, 0x61,
        0x69, 0x6E, 0x5F, 0x5F, 0x00, 0x4D, 0x06, 0x00,
        0x1F, 0x14, 0x01, 0x0E, 0x74, 0x65, 0x73, 0x74,
        0x2E, 0x70, 0x79, 0x00, 0x0F, 0x10, 0x45, 0x56,
        0x33, 0x42, 0x72, 0x69, 0x63, 0x6B, 0x00, 0x1A,
        0x70, 0x79, 0x62, 0x72, 0x69, 0x63, 0x6B, 0x73,
        0x2E, 0x68, 0x75, 0x62, 0x73, 0x00, 0x0A, 0x43,
        0x6F, 0x6C, 0x6F, 0x72, 0x00, 0x26, 0x70, 0x79,
        0x62, 0x72, 0x69, 0x63, 0x6B, 0x73, 0x2E, 0x70,
        0x61, 0x72, 0x61, 0x6D, 0x65, 0x74, 0x65, 0x72,
        0x73, 0x00, 0x08, 0x77, 0x61, 0x69, 0x74, 0x00,
        0x1C, 0x70, 0x79, 0x62, 0x72, 0x69, 0x63, 0x6B,
        0x73, 0x2E, 0x74, 0x6F, 0x6F, 0x6C, 0x73, 0x00,
        0x0C, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6D, 0x00,
        0x0E, 0x73, 0x74, 0x6F, 0x72, 0x61, 0x67, 0x65,
        0x00, 0x81, 0x7B, 0x82, 0x49, 0x0A, 0x6C, 0x69,
        0x67, 0x68, 0x74, 0x00, 0x04, 0x6F, 0x6E, 0x00,
        0x06, 0x52, 0x45, 0x44, 0x00, 0x0A, 0x47, 0x52,
        0x45, 0x45, 0x4E, 0x00, 0x06, 0x68, 0x75, 0x62,
        0x00, 0x81, 0x15, 0x81, 0x05, 0x81, 0x77, 0x05,
        0x11, 0x50, 0x65, 0x72, 0x73, 0x69, 0x73, 0x74,
        0x65, 0x6E, 0x74, 0x20, 0x63, 0x6F, 0x75, 0x6E,
        0x74, 0x3A, 0x00, 0x89, 0x58, 0x30, 0x18, 0x01,
        0x2C, 0x2C, 0x4C, 0x46, 0x31, 0x5B, 0x49, 0x20,
        0x2D, 0x28, 0x2D, 0x80, 0x10, 0x02, 0x2A, 0x01,
        0x1B, 0x03, 0x1C, 0x02, 0x16, 0x02, 0x59, 0x80,
        0x10, 0x04, 0x2A, 0x01, 0x1B, 0x05, 0x1C, 0x04,
        0x16, 0x04, 0x59, 0x80, 0x10, 0x06, 0x2A, 0x01,
        0x1B, 0x07, 0x1C, 0x06, 0x16, 0x06, 0x59, 0x11,
        0x02, 0x34, 0x00, 0x16, 0x10, 0x11, 0x10, 0x13,
        0x08, 0x14, 0x09, 0x80, 0x10, 0x0A, 0x81, 0x36,
        0x82, 0x01, 0x80, 0x55, 0x16, 0x11, 0x11, 0x10,
        0x13, 0x08, 0x14, 0x09, 0x80, 0x10, 0x0B, 0x11,
        0x12, 0x11, 0x11, 0x81, 0xF2, 0x22, 0x82, 0x00,
        0xF8, 0x2B, 0x01, 0x34, 0x01, 0x36, 0x82, 0x01,
        0x59, 0x11, 0x13, 0x23, 0x00, 0x11, 0x11, 0x34,
        0x02, 0x59, 0x11, 0x10, 0x13, 0x0C, 0x14, 0x0D,
        0x11, 0x04, 0x13, 0x0E, 0x36, 0x01, 0x59, 0x11,
        0x06, 0x22, 0x83, 0x74, 0x34, 0x01, 0x59, 0x11,
        0x10, 0x13, 0x0C, 0x14, 0x0D, 0x11, 0x04, 0x13,
        0x0F, 0x36, 0x01, 0x59, 0x11, 0x06, 0x22, 0x83,
        0x74, 0x34, 0x01, 0x59, 0x42, 0x14, 0x51, 0x63,
    };

    // The block device normally doesn't need to be aware of this higher level
    // data structure, but we use it here to mimic some working data into memory
    // so we have something to test against.
    static struct {
        uint32_t write_size;
        uint8_t user_data[PBSYS_CONFIG_STORAGE_USER_DATA_SIZE];
        char stored_firmware_hash[8];
        pbsys_storage_settings_t settings;
        uint32_t program_offset;
        uint32_t program_size;
        uint8_t program_data[sizeof(_program_data)];
    } disk = { 0 };

    // Prepare fake disk data
    disk.write_size = sizeof(disk) + sizeof(_program_data);
    disk.program_size = sizeof(_program_data);
    memcpy(&disk.stored_firmware_hash[0], MICROPY_GIT_HASH, sizeof(disk.stored_firmware_hash));
    memcpy(&disk.program_data[0], _program_data, sizeof(_program_data));

    // Initial value of that one user byte:
    disk.user_data[0] = 123;

    // Copy requested data to RAM.
    memcpy(buffer, (uint8_t *)&disk + offset, size);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size) {

    // static pbio_os_state_t sub;
    // static uint32_t size_done;
    // static uint32_t size_now;
    // pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // REVSISIT: DELETE ME!
    return delete_me_load_placeholder_data(offset, buffer, size);

    // // Exit on invalid size.
    // if (size == 0 || offset + size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
    //     return PBIO_ERROR_INVALID_ARG;
    // }

    // // Split up reads to maximum chunk size.
    // for (size_done = 0; size_done < size; size_done += size_now) {
    //     size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_READ);

    //     // Set address for this read request and send it.
    //     set_address_be(&cmd_request_read.buffer[1], PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + offset + size_done);
    //     PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_request_read));
    //     if (err != PBIO_SUCCESS) {
    //         return err;
    //     }

    //     // Receive the data.
    //     cmd_data_read.buffer = buffer + size_done;
    //     cmd_data_read.size = size_now;
    //     PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_read));
    //     if (err != PBIO_SUCCESS) {
    //         return err;
    //     }
    // }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// /**
//  * Write or erase one chunk of data from flash.
//  *
//  * In case of write, address must be aligned with a page.
//  *
//  * In case of erase (indicated by size = 0), address must be aligned with a sector.
//  */
// static pbio_error_t flash_erase_or_write(pbio_os_state_t *state, uint32_t address, uint8_t *buffer, uint32_t size) {

//     static pbio_os_state_t sub;
//     static const spi_command_t *cmd;
//     pbio_error_t err;

//     PBIO_OS_ASYNC_BEGIN(state);

//     // Enable write mode.
//     PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_write_enable));
//     if (err != PBIO_SUCCESS) {
//         return err;
//     }

//     // Select either write or erase request.
//     cmd = size == 0 ? &cmd_request_erase : &cmd_request_write;

//     // Set address and send the request.
//     set_address_be(&cmd->buffer[1], address);
//     PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, cmd));
//     if (err != PBIO_SUCCESS) {
//         return err;
//     }

//     // Write the data, or skip in case of erase.
//     if (size != 0) {
//         cmd_data_write.buffer = buffer;
//         cmd_data_write.size = size;
//         PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_write));
//         if (err != PBIO_SUCCESS) {
//             return err;
//         }
//     }

//     // Wait for busy flag to clear.
//     do {
//         // Send command to read status.
//         PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_tx));
//         if (err != PBIO_SUCCESS) {
//             return err;
//         }

//         // Read the status.
//         PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_rx));
//         if (err != PBIO_SUCCESS) {
//             return err;
//         }
//     } while (status & (FLASH_STATUS_BUSY | FLASH_STATUS_WRITE_ENABLED));

//     // The task is ready.
//     PBIO_OS_ASYNC_END(PBIO_SUCCESS);
// }

pbio_error_t pbdrv_block_device_store(pbio_os_state_t *state, uint8_t *buffer, uint32_t size) {

    // static pbio_os_state_t sub;
    // static uint32_t offset;
    // static uint32_t size_now;
    // static uint32_t size_done;
    // pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // // Exit on invalid size.
    // if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
    //     return PBIO_ERROR_INVALID_ARG;
    // }

    // // Erase sector by sector.
    // for (offset = 0; offset < size; offset += FLASH_SIZE_ERASE) {
    //     // Writing size 0 means erase.
    //     PBIO_OS_AWAIT(state, &sub, err = flash_erase_or_write(&sub,
    //         PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + offset, NULL, 0));
    //     if (err != PBIO_SUCCESS) {
    //         return err;
    //     }
    // }

    // // Write page by page.
    // for (size_done = 0; size_done < size; size_done += size_now) {
    //     size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_WRITE);
    //     PBIO_OS_AWAIT(state, &sub, err = flash_erase_or_write(&sub,
    //         PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + size_done, buffer + size_done, size_now));
    //     if (err != PBIO_SUCCESS) {
    //         return err;
    //     }
    // }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

uint8_t tx_test[4] = {0x9f, 0x5a, 0xa5, 0x33};
uint8_t rx_test[4] = {0xde, 0xad, 0xbe, 0xef};
uint32_t tx_test_blahblah;

static pbio_os_process_t pbdrv_block_device_ev3_init_process;

pbio_error_t pbdrv_block_device_ev3_init_process_thread(pbio_os_state_t *state, void *context) {

    pbdrv_uart_debug_printf("block device init thread\r\n");

    // pbio_error_t err;
    // static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    spi0_setup_for_flash();

    tx_test_blahblah = spi0_last_dat1_for_flash(tx_test[3]);

    EDMA3CCPaRAMEntry_ ps;
    ps.p.srcAddr = (unsigned int)tx_test;
    ps.p.destAddr = SOC_SPI_0_REGS + SPI_SPIDAT1;
    ps.p.aCnt = 1;
    ps.p.bCnt = 4 - 1;
    ps.p.cCnt = 1;
    ps.p.srcBIdx = 1;
    ps.p.destBIdx = 0;
    ps.p.srcCIdx = 0;
    ps.p.destCIdx = 0;
    ps.p.linkAddr = 127 * 32;
    ps.p.bCntReload = 0;
    ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_TX << EDMA3CC_OPT_TCC_SHIFT);
    edma3_set_param(EDMA3_CHA_SPI0_TX, &ps);

    ps.p.srcAddr = (unsigned int)&tx_test_blahblah;
    ps.p.destAddr = SOC_SPI_0_REGS + SPI_SPIDAT1;
    ps.p.aCnt = 4;
    ps.p.bCnt = 1;
    ps.p.linkAddr = 0xffff;
    edma3_set_param(127, &ps);
    
    ps.p.srcAddr = SOC_SPI_0_REGS + SPI_SPIBUF;
    ps.p.destAddr = (unsigned int)rx_test;
    ps.p.aCnt = 1;
    ps.p.bCnt = 4;
    ps.p.cCnt = 1;
    ps.p.srcBIdx = 0;
    ps.p.destBIdx = 1;
    ps.p.srcCIdx = 0;
    ps.p.destCIdx = 0;
    ps.p.linkAddr = 0xffff;
    ps.p.bCntReload = 0;
    ps.p.opt = EDMA3CC_OPT_TCINTEN | (EDMA3_CHA_SPI0_RX << EDMA3CC_OPT_TCC_SHIFT);
    edma3_set_param(EDMA3_CHA_SPI0_RX, &ps);

    bdev.spi_status = SPI_STATUS_WAIT_TX | SPI_STATUS_WAIT_RX;

    pbdrv_uart_debug_printf("block device init thread param setup\r\n");

    // TODO: pbio probably needs a framework for memory barriers and DMA cache management
    __asm__ volatile("":::"memory");

    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI0_TX, EDMA3_TRIG_MODE_EVENT);
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_SPI0_RX, EDMA3_TRIG_MODE_EVENT);
    SPIIntEnable(SOC_SPI_0_REGS, (SPI_DMA_REQUEST_ENA_INT));

    PBIO_OS_AWAIT_UNTIL(state, !(bdev.spi_status & SPI_STATUS_WAIT_ANY));

    __asm__ volatile("":::"memory");

    pbdrv_uart_debug_printf("block device init thread wait done\r\n");
    pbdrv_uart_debug_printf("%02x%02x%02x%02x\r\n", rx_test[0] & 0xff, rx_test[1] & 0xff, rx_test[2] & 0xff, rx_test[3] & 0xff);

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x9f;
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x5a;
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0xa5;
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // // *(volatile uint16_t *)(SOC_SPI_0_REGS + SPI_SPIDAT1 + 2) = 1 << 3;
    // // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x33;
    // HWREG(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x33 | (1 << (8 + 3));
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x\r\n", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // spi0_setup_for_flash();

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x9f;
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x5a;
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0xa5;
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_TXINTFLG)) {}
    // // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1 + 3) = 0;
    // *(volatile uint16_t *)(SOC_SPI_0_REGS + SPI_SPIDAT1 + 2) = 1 << 3;
    // *(volatile unsigned char *)(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x33;
    // // HWREG(SOC_SPI_0_REGS + SPI_SPIDAT1) = 0x33 | (1 << (8 + 3));
    // while (!(HWREG(SOC_SPI_0_REGS + SPI_SPIFLG) & SPI_SPIFLG_RXINTFLG)) {}
    // pbdrv_uart_debug_printf("%02x\r\n", HWREG(SOC_SPI_0_REGS + SPI_SPIBUF) & 0xff);

    // // Write the ID getter command
    // PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_tx));
    // if (err != PBIO_SUCCESS) {
    //     return err;
    // }

    // // Get ID command reply
    // PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_rx));
    // if (err != PBIO_SUCCESS) {
    //     return err;
    // }

    // // Verify flash device ID // REVISIT: Fix up id_data so we can memcmp
    // // if (memcmp(device_id, id_data, sizeof(id_data))) {
    // //     return PBIO_ERROR_FAILED;
    // // }

    // Initialization done.
    pbdrv_init_busy_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_block_device_init(void) {

    pbdrv_uart_debug_printf("block device init test\r\n");

    // SPI module basic init
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_SPI0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    SPIReset(SOC_SPI_0_REGS);
    SPIOutOfReset(SOC_SPI_0_REGS);
    SPIModeConfigure(SOC_SPI_0_REGS, SPI_MASTER_MODE);
    unsigned int  spipc0 = SPI_SPIPC0_SOMIFUN | SPI_SPIPC0_SIMOFUN | SPI_SPIPC0_CLKFUN | SPI_SPIPC0_SCS0FUN0 | SPI_SPIPC0_SCS0FUN3;
    SPIPinControl(SOC_SPI_0_REGS, 0, 0, &spipc0);
    SPIDefaultCSSet(SOC_SPI_0_REGS, (1 << 0) | (1 << 3));

    // SPI module data formats
    SPIClkConfigure(SOC_SPI_0_REGS, SOC_SPI_0_MODULE_FREQ, SPI_CLK_SPEED_FLASH, SPI_DATA_FORMAT0);
    SPIConfigClkFormat(SOC_SPI_0_REGS, SPI_CLK_POL_HIGH | SPI_CLK_INPHASE, SPI_DATA_FORMAT0);
    SPIShiftMsbFirst(SOC_SPI_0_REGS, SPI_DATA_FORMAT0);
    SPICharLengthSet(SOC_SPI_0_REGS, 8, SPI_DATA_FORMAT0);
    // TODO: ADC data format

    // Configure the GPIO pins.
    pbdrv_gpio_alt(&pin_spi0_mosi, SYSCFG_PINMUX3_PINMUX3_15_12_SPI0_SIMO0);
    pbdrv_gpio_alt(&pin_spi0_miso, SYSCFG_PINMUX3_PINMUX3_11_8_SPI0_SOMI0);
    pbdrv_gpio_alt(&pin_spi0_clk, SYSCFG_PINMUX3_PINMUX3_3_0_SPI0_CLK);
    pbdrv_gpio_alt(&pin_spi0_ncs0, SYSCFG_PINMUX4_PINMUX4_7_4_NSPI0_SCS0);

    // Configure the flash control pins and put them with the values we want
    pbdrv_gpio_alt(&pin_flash_nwp, SYSCFG_PINMUX12_PINMUX12_23_20_GPIO5_2);
    pbdrv_gpio_out_high(&pin_flash_nwp);
    pbdrv_gpio_alt(&pin_flash_nhold, SYSCFG_PINMUX6_PINMUX6_31_28_GPIO2_0);
    pbdrv_gpio_out_high(&pin_flash_nhold);

    // TODO: We currently disable the ADC CS
    pbdrv_gpio_alt(&pin_spi0_ncs3, SYSCFG_PINMUX3_PINMUX3_27_24_GPIO8_2);
    pbdrv_gpio_out_high(&pin_spi0_ncs3);

    // Set up interrupts
    SPIIntLevelSet(SOC_SPI_0_REGS, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);
    
    // Request DMA channels. This only needs to be done for the initial events (and not for chained parameter sets)
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, EDMA3_CHA_SPI0_TX, EDMA3_CHA_SPI0_TX, 0);
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, EDMA3_CHA_SPI1_RX, EDMA3_CHA_SPI1_RX, 0); 

    // Enable!
    SPIEnable(SOC_SPI_0_REGS);

    pbdrv_uart_debug_printf("block device init done basic\r\n");

    bdev.spi_status = SPI_STATUS_COMPLETE;

    pbdrv_init_busy_up();
    pbio_os_process_start(&pbdrv_block_device_ev3_init_process, pbdrv_block_device_ev3_init_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_EV3

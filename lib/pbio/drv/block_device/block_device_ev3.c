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

#include <tiam1808/spi.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#include "../drv/gpio/gpio_ev3.h"

#include <pbio/error.h>
#include <pbio/int_math.h>

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

/**
 * The block device driver state.
 */
static struct {
    /** HAL Transfer status */
    volatile spi_status_t spi_status;
    //
    // Any state variables can go here as needed. We don't need pdata in platform.c
    // since this driver is going to only ever be used with EV3
    //
} bdev;


/**
 * Tx transfer complete. // Leaving this here for inspiration as hook for TI API. Delete if not needed.
 */
void pbdrv_block_device_ev3_spi_tx_complete(void) {
    bdev.spi_status = SPI_STATUS_COMPLETE;
    pbio_os_request_poll();
}

/**
 * Rx transfer complete. // Leaving this here for inspiration as hook for TI API. Delete if not needed.
 */
void pbdrv_block_device_ev3_spi_rx_complete(void) {
    bdev.spi_status = SPI_STATUS_COMPLETE;
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

// LCD SPI0 Clock
static const pbdrv_gpio_t pin_spi0_clk = PBDRV_GPIO_EV3_PIN(3, 3, 0, 1, 8);

// ADC / Flash SPI0 chip select (active low).
static const pbdrv_gpio_t pin_spi0_ncs3 = PBDRV_GPIO_EV3_PIN(3, 27, 24, 8, 2);
static const pbdrv_gpio_t pin_spi0_ncs0 = PBDRV_GPIO_EV3_PIN(4, 7, 4, 1, 6);

/**
 * Sets or clears the chip select line.
 *
 * // REVISIT: TI API also seems to have ways of having the peripheral do this for us. Whatever works is fine.
 *
 * @param [in] set         Whether to set (true) or clear (false) CS.
 */
static void spi_chip_select(bool set) {
    // Active low, so set CS means set /CS low.
    if (set) {
        pbdrv_gpio_out_low(&pin_spi0_ncs0);
    } else {
        pbdrv_gpio_out_high(&pin_spi0_ncs0);
    }
}

/**
 * Initiates an SPI transfer via DMA.
 *
 * @param [in] cmd         The command to start.
 * @return                 ::PBIO_SUCCESS on success.
 *                         ::PBIO_ERROR_BUSY if SPI is busy.
 *                         ::PBIO_ERROR_INVALID_ARG for invalid args to HAL call.
 *                         ::PBIO_ERROR_IO for other errors.
 */
static pbio_error_t spi_begin(const spi_command_t *cmd) {

    if (bdev.spi_status == SPI_STATUS_WAIT) {
        // Another read operation is already in progress.
        return PBIO_ERROR_BUSY;
    }
    if (bdev.spi_status == SPI_STATUS_ERROR) {
        // Previous transmission went wrong.
        return PBIO_ERROR_IO;
    }
    // Set status to wait and start receiving.
    bdev.spi_status = SPI_STATUS_WAIT;

    // REPLACE WITH TI AM1808 SPI DMA operation
    //
    //
    // // Start SPI operation.
    // HAL_StatusTypeDef err;
    // if (cmd->operation == SPI_RECV) {
    //     err = HAL_SPI_Receive_DMA(&bdev.hspi, cmd->buffer, cmd->size);
    // } else {
    //     err = HAL_SPI_Transmit_DMA(&bdev.hspi, cmd->buffer, cmd->size);
    // }

    // // Handle HAL errors.
    // switch (err) {
    //     case HAL_OK:
    //         return PBIO_SUCCESS;
    //     case HAL_ERROR:
    //         return PBIO_ERROR_INVALID_ARG;
    //     case HAL_BUSY:
    //         return PBIO_ERROR_BUSY;
    //     default:
    //         return PBIO_ERROR_IO;
    // }

    // REVISIT: DELETE ME. Here for now until we implement a real transfer.
    bdev.spi_status = SPI_STATUS_COMPLETE;
    return PBIO_SUCCESS;
}

/**
 * Starts and awaits an SPI transfer.
 */
static pbio_error_t spi_command_thread(pbio_os_state_t *state, const spi_command_t *cmd) {

    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Select peripheral.
    spi_chip_select(true);

    // Start SPI operation.
    err = spi_begin(cmd);
    if (err != PBIO_SUCCESS) {
        spi_chip_select(false);
        return err;
    }

    // Wait until SPI operation completes.
    PBIO_OS_AWAIT_UNTIL(state, bdev.spi_status == SPI_STATUS_COMPLETE);

    // Turn off peripheral if requested.
    if (!(cmd->operation & SPI_CS_KEEP_ENABLED)) {
        spi_chip_select(false);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

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

static void set_address_be(uint8_t *buf, uint32_t address) {
    buf[0] = address >> 16;
    buf[1] = address >> 8;
    buf[2] = address;
}

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

// W25Qxx manufacturer and device ID.  // REVISIT: for N25Q128
static const uint8_t device_id[] = {0xEF, 0x40, W25Qxx(0x16, 0x19)};

// Request flash device ID.
static const spi_command_t cmd_id_tx = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = &(uint8_t) {FLASH_CMD_GET_ID},
    .size = 1,
};

// Receive flash device ID after sending request.
static uint8_t id_data[sizeof(device_id)];
static const spi_command_t cmd_id_rx = {
    .operation = SPI_RECV,
    .buffer = id_data,
    .size = sizeof(id_data),
};

// Enable flash writing.
static const spi_command_t cmd_write_enable = {
    .operation = SPI_SEND,
    .buffer = &(uint8_t) {FLASH_CMD_WRITE_ENABLE},
    .size = 1,
};

// Request the write-status byte.
static const spi_command_t cmd_status_tx = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = &(uint8_t) {FLASH_CMD_GET_STATUS},
    .size = 1,
};

// Get the write-status byte.
static uint8_t status;
static const spi_command_t cmd_status_rx = {
    .operation = SPI_RECV,
    .buffer = (uint8_t *)&status,
    .size = sizeof(status),
};

// Request reading from address. Buffer: read command + address + dummy byte.
// Should be followed by another command that reads the data.
static uint8_t read_address[1 + FLASH_ADDRESS_SIZE + 1] = {FLASH_CMD_READ_DATA};
static const spi_command_t cmd_request_read = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = read_address,
    .size = sizeof(read_address),
};

// Request page write at address. Buffer: write command + address.
// Should be followed by cmd_data_read to read the data.
static uint8_t write_address[1 + FLASH_ADDRESS_SIZE] = {FLASH_CMD_WRITE_DATA};
static const spi_command_t cmd_request_write = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = write_address,
    .size = sizeof(write_address),
};

// Request sector erase at address. Buffer: erase command + address.
static uint8_t erase_address[1 + FLASH_ADDRESS_SIZE] = {FLASH_CMD_ERASE_BLOCK};
static const spi_command_t cmd_request_erase = {
    .operation = SPI_SEND,
    .buffer = erase_address,
    .size = sizeof(erase_address),
};

// Transmit data. Buffer and size should be set wherever this is used.
static spi_command_t cmd_data_write = {
    .operation = SPI_SEND,
};

// Read data. Buffer and size should be set wherever this is used.
static spi_command_t cmd_data_read = {
    .operation = SPI_RECV,
};

pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size) {

    static pbio_os_state_t sub;
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
        set_address_be(&cmd_request_read.buffer[1], PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + offset + size_done);
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_request_read));
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // Receive the data.
        cmd_data_read.buffer = buffer + size_done;
        cmd_data_read.size = size_now;
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_read));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Write or erase one chunk of data from flash.
 *
 * In case of write, address must be aligned with a page.
 *
 * In case of erase (indicated by size = 0), address must be aligned with a sector.
 */
static pbio_error_t flash_erase_or_write(pbio_os_state_t *state, uint32_t address, uint8_t *buffer, uint32_t size) {

    static pbio_os_state_t sub;
    static const spi_command_t *cmd;
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Enable write mode.
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_write_enable));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Select either write or erase request.
    cmd = size == 0 ? &cmd_request_erase : &cmd_request_write;

    // Set address and send the request.
    set_address_be(&cmd->buffer[1], address);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, cmd));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Write the data, or skip in case of erase.
    if (size != 0) {
        cmd_data_write.buffer = buffer;
        cmd_data_write.size = size;
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_write));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Wait for busy flag to clear.
    do {
        // Send command to read status.
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_tx));
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // Read the status.
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_rx));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } while (status & (FLASH_STATUS_BUSY | FLASH_STATUS_WRITE_ENABLED));

    // The task is ready.
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

    // Erase sector by sector.
    for (offset = 0; offset < size; offset += FLASH_SIZE_ERASE) {
        // Writing size 0 means erase.
        PBIO_OS_AWAIT(state, &sub, err = flash_erase_or_write(&sub,
            PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + offset, NULL, 0));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Write page by page.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_WRITE);
        PBIO_OS_AWAIT(state, &sub, err = flash_erase_or_write(&sub,
            PBDRV_CONFIG_BLOCK_DEVICE_EV3_START_ADDRESS + size_done, buffer + size_done, size_now));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_os_process_t pbdrv_block_device_ev3_init_process;

pbio_error_t pbdrv_block_device_ev3_init_process_thread(pbio_os_state_t *state, void *context) {

    pbio_error_t err;
    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // Write the ID getter command
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_tx));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get ID command reply
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_rx));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Verify flash device ID // REVISIT: Fix up id_data so we can memcmp
    // if (memcmp(device_id, id_data, sizeof(id_data))) {
    //     return PBIO_ERROR_FAILED;
    // }

    // Initialization done.
    pbdrv_init_busy_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_block_device_init(void) {

    bdev.spi_status = SPI_STATUS_COMPLETE;

    // Configure the GPIO pins.
    pbdrv_gpio_alt(&pin_spi0_mosi, SYSCFG_PINMUX3_PINMUX3_15_12_SPI0_SIMO0);
    pbdrv_gpio_alt(&pin_spi0_miso, SYSCFG_PINMUX3_PINMUX3_11_8_SPI0_SOMI0);
    pbdrv_gpio_alt(&pin_spi0_clk, SYSCFG_PINMUX3_PINMUX3_3_0_SPI0_CLK);

    // REVISIT: Do we want to have the peripheral control CS or do it manually
    // like we did in the W25QXX STM32 driver? If so, just set it high just
    // like the next pin below.
    pbdrv_gpio_alt(&pin_spi0_ncs0, SYSCFG_PINMUX4_PINMUX4_7_4_NSPI0_SCS0);

    // Is this sufficient to disable ADC SPI?
    pbdrv_gpio_out_high(&pin_spi0_ncs3);

    //
    // REVISIT: Init SPI and DMA with TI AM1808 API
    //
    // See display_ev3.c for inspiration and adapt settings as needed.
    //

    pbdrv_init_busy_up();
    pbio_os_process_start(&pbdrv_block_device_ev3_init_process, pbdrv_block_device_ev3_init_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_EV3

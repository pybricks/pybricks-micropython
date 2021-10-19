// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors



#include "py/mpconfig.h"

#if PYBRICKS_HUB_PRIMEHUB

#include <string.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include <stm32f4xx_ll_spi.h>
#include <stm32f4xx_hal.h>

#include "py/mphal.h"

#include "lfs.h"

#include <pybricks/util_pb/pb_flash.h>


#define FLASH_SIZE_TOTAL (0x2000000)
#define FLASH_SIZE_BOOT  (0x100000)
#define FLASH_SIZE_USER  (FLASH_SIZE_TOTAL - FLASH_SIZE_BOOT)

#define FLASH_TIMEOUT (100)

enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_WRITE_DATA = 0x12,
    FLASH_CMD_READ_DATA = 0x13,
    FLASH_CMD_ERASE_BLOCK = 0x21,
    FLASH_CMD_GET_ID = 0x9F,
};

enum {
    FLASH_STATUS_BUSY = 0x01,
    FLASH_STATUS_WRITE_ENABLED = 0x02,
};

SPI_HandleTypeDef hspi2;

// Whether the SPI and filesystem have been initialized once after boot.
static bool flash_initialized = false;

// True sets the notCS pin, False resets it
static void flash_enable(bool enable) {
    if (enable) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    }
}

// Configure SPI
static pbio_error_t pb_flash_spi_init(void) {

    // SPI2 parameter configuration
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;

    // Save settings
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        return PBIO_ERROR_IO;
    }

    // Enable flash
    flash_enable(true);

    // Command parameters
    uint8_t command = FLASH_CMD_GET_ID;
    uint8_t id_data[3];
    HAL_StatusTypeDef err;

    // Send ID command
    err = HAL_SPI_Transmit(&hspi2, &command, sizeof(command), FLASH_TIMEOUT);
    if (err != 0) {
        return PBIO_ERROR_IO;
    }

    // Get ID command reply
    err = HAL_SPI_Receive(&hspi2, id_data, sizeof(id_data), FLASH_TIMEOUT);
    if (err != 0) {
        return PBIO_ERROR_IO;
    }
    flash_enable(false);

    // Verify flash device ID
    if (id_data[0] != 239 || id_data[1] != 64 || id_data[2] != 25) {
        return PBIO_ERROR_NO_DEV;
    }

    return PBIO_SUCCESS;
}

static HAL_StatusTypeDef flash_status_read(uint8_t *status) {

    // Read status command
    uint8_t command = FLASH_CMD_GET_STATUS;

    // Write the read status command
    flash_enable(true);
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, &command, sizeof(command), FLASH_TIMEOUT);
    if (err != HAL_OK) {
        return err;
    }

    // Receive data
    err = HAL_SPI_Receive(&hspi2, status, 1, FLASH_TIMEOUT);
    flash_enable(false);
    return err;
}

static HAL_StatusTypeDef flash_wait_ready() {
    uint8_t status = FLASH_STATUS_BUSY | FLASH_STATUS_WRITE_ENABLED;
    HAL_StatusTypeDef err;
    uint32_t start_time = mp_hal_ticks_ms();

    // While write enabled and / or busy, wait
    while (status & (FLASH_STATUS_BUSY | FLASH_STATUS_WRITE_ENABLED)) {
        err = flash_status_read(&status);
        if (err != HAL_OK) {
            return err;
        }
        if (mp_hal_ticks_ms() - start_time > FLASH_TIMEOUT) {
            return HAL_TIMEOUT;
        }
        MICROPY_EVENT_POLL_HOOK;
    }
    return HAL_OK;
}

static HAL_StatusTypeDef flash_send_address_command(uint8_t command, uint32_t address) {

    // Can only read/write user partition
    if (address > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }

    // Pack command and address as big endian. This is the absolute address,
    // starting in the boot partition.
    uint8_t data[5] = {command};
    pbio_set_uint32_be(&data[1], address);

    // Enable flash
    flash_enable(true);

    // Send address command
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, data, sizeof(data), FLASH_TIMEOUT);
    if (err != HAL_OK) {
        flash_enable(false);
        return err;
    }
    return err;
}

static HAL_StatusTypeDef flash_read(uint32_t address, uint8_t *buffer, uint32_t size) {

    if (address + size > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }

    // Enable flash and send the read command with address, offset by boot partition
    HAL_StatusTypeDef err = flash_send_address_command(FLASH_CMD_READ_DATA, address + FLASH_SIZE_BOOT);

    // Receive data
    err = HAL_SPI_Receive(&hspi2, buffer, size, FLASH_TIMEOUT);
    flash_enable(false);
    return err;
}

static HAL_StatusTypeDef write_enable() {
    uint8_t command = FLASH_CMD_WRITE_ENABLE;
    flash_enable(true);
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, &command, sizeof(command), FLASH_TIMEOUT);
    flash_enable(false);
    return err;
}

static HAL_StatusTypeDef flash_write(uint32_t address, const uint8_t *buffer, uint32_t size) {

    if (address + size > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }

    // Enable write mode
    HAL_StatusTypeDef err = write_enable();
    if (err != HAL_OK) {
        return err;
    }

    // Enable flash and send the write command with address, offset by boot partition
    err = flash_send_address_command(FLASH_CMD_WRITE_DATA, address + FLASH_SIZE_BOOT);

    // Write the data
    err = HAL_SPI_Transmit(&hspi2, (uint8_t *)buffer, size, FLASH_TIMEOUT);
    flash_enable(false);
    if (err != HAL_OK) {
        return err;
    }
    return flash_wait_ready();
}

static HAL_StatusTypeDef flash_block_erase(uint32_t address) {

    if (address > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }

    // Enable write mode
    HAL_StatusTypeDef err = write_enable();
    if (err != HAL_OK) {
        return err;
    }

    // Enable flash and send the erase block command with address, offset by boot partition
    err = flash_send_address_command(FLASH_CMD_ERASE_BLOCK, address + FLASH_SIZE_BOOT);
    flash_enable(false);
    if (err != HAL_OK) {
        return err;
    }
    return flash_wait_ready();
}

static int block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {

    lfs_size_t done = 0;

    while (done < size) {

        // How many bytes to read in one go.
        lfs_size_t read_now = size - done;
        if (read_now > c->read_size) {
            read_now = c->read_size;
        }

        // Read chunk of flash.
        if (flash_read(block * c->block_size + off + done, buffer + done, read_now) != HAL_OK) {
            return LFS_ERR_IO;
        }
        done += read_now;

        // Give MicroPython and PBIO some time.
        MICROPY_EVENT_POLL_HOOK;
    }
    return 0;
}

static int block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    lfs_size_t done = 0;

    while (done < size) {

        // How many bytes to write in one go.
        lfs_size_t write_now = size - done;

        // Must not be larger than write size
        if (write_now > c->prog_size) {
            write_now = c->prog_size;
        }

        // Must not wrap around the page
        lfs_size_t write_max = 256 - ((off + done) & 0xFF);
        if (write_now > write_max) {
            write_now = write_max;
        }

        // Write chunk of flash.
        if (flash_write(block * c->block_size + off + done, buffer + done, write_now) != HAL_OK) {
            return LFS_ERR_IO;
        }
        done += write_now;

        // Give MicroPython and PBIO some time.
        MICROPY_EVENT_POLL_HOOK;
    }

    return 0;
}

static int block_device_erase(const struct lfs_config *c, lfs_block_t block) {
    // Erase block of flash. Block size assumed to match erase size.

    if (flash_block_erase(block * c->block_size) != HAL_OK) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

static int block_device_sync(const struct lfs_config *c) {
    return 0;
}

static lfs_t lfs;
static lfs_file_t file;

static uint8_t lfs_read_buf[256];
static uint8_t lfs_prog_buf[256];
static uint8_t lfs_lookahead_buf[256];
static uint8_t lfs_file_buf[256];

static const struct lfs_config cfg = {
    .read = block_device_read,
    .prog = block_device_prog,
    .erase = block_device_erase,
    .sync = block_device_sync,

    .read_size = 256,
    .prog_size = 256,
    .block_size = 4096,
    .block_count = 7936,
    .lookahead = sizeof(lfs_lookahead_buf) * 8,

    .read_buffer = lfs_read_buf,
    .prog_buffer = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
    .file_buffer = lfs_file_buf,
};

pbio_error_t pb_flash_init(void) {

    // Initialize only once
    if (flash_initialized) {
        return PBIO_SUCCESS;
    }

    // Init SPI
    pbio_error_t err = pb_flash_spi_init();
    if (err != PBIO_SUCCESS) {
        return err;
    }

    uint8_t lfs_data[58];

    // Read littlefs data
    if (flash_read(0, lfs_data, sizeof(lfs_data)) != HAL_OK) {
        return PBIO_ERROR_IO;
    }

    // Verify magic string for littlefs V1. Anything else is not supported.
    const char *magic = "littlefs";
    if (memcmp(&lfs_data[40], magic, sizeof(magic) - 1) != 0) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    // Verify block parameters and on-flash version of littlefs
    uint32_t block_size = pbio_get_uint32_le(&lfs_data[28]);
    uint32_t block_count = pbio_get_uint32_le(&lfs_data[32]);
    uint32_t version = pbio_get_uint32_le(&lfs_data[36]);

    if (cfg.block_size != block_size || cfg.block_count != block_count || version != 0x00010001) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    // Mount file system
    if (lfs_mount(&lfs, &cfg) != LFS_ERR_OK) {
        return PBIO_ERROR_FAILED;
    }

    // Ensure there is a _pybricks system folder
    int lfs_err = lfs_mkdir(&lfs, "_pybricks");
    if (!(lfs_err == LFS_ERR_OK || lfs_err == LFS_ERR_EXIST)) {
        return PBIO_ERROR_FAILED;
    }

    // We're ready to read and write now.
    flash_initialized = true;

    // The first write takes a bit longer, so do it now.
    const uint8_t *pybricks_magic = (const uint8_t *)"pybricks";
    return pb_flash_file_write("_pybricks/boot.txt", pybricks_magic, sizeof(pybricks_magic) - 1);
}

pbio_error_t pb_flash_raw_read(uint32_t address, uint8_t *buf, uint32_t size) {

    if (flash_read(address, buf, size) != HAL_OK) {
        return PBIO_ERROR_FAILED;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pb_flash_file_open_get_size(const char *path, uint32_t *size) {

    // Check that flash was initialized
    if (!flash_initialized) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Open file
    if (lfs_file_open(&lfs, &file, path, LFS_O_RDONLY) != LFS_ERR_OK) {
        return PBIO_ERROR_FAILED;
    }
    *size = file.size;
    return PBIO_SUCCESS;
}

pbio_error_t pb_flash_file_read(uint8_t *buf, uint32_t size) {

    // Check that flash was initialized
    if (!flash_initialized) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Allow only read in one go for now
    if (size != file.size) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Read whole file to buffer
    if (lfs_file_read(&lfs, &file, buf, size) != size) {
        return PBIO_ERROR_FAILED;
    }

    // Close the file
    if (lfs_file_close(&lfs, &file) != LFS_ERR_OK) {
        return PBIO_ERROR_FAILED;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pb_flash_file_write(const char *path, const uint8_t *buf, uint32_t size) {

    // Check that flash was initialized
    if (!flash_initialized) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Open file
    if (lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
        return PBIO_ERROR_FAILED;
    }

    // write file contents
    if (lfs_file_write(&lfs, &file, buf, size) != size) {
        return PBIO_ERROR_FAILED;
    }

    // Close the file
    if (lfs_file_close(&lfs, &file) != LFS_ERR_OK) {
        return PBIO_ERROR_FAILED;
    }
    return PBIO_SUCCESS;
}

#endif // PYBRICKS_HUB_PRIMEHUB

// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors



#include "py/mpconfig.h"

#if (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)

#include <string.h>

#include <pbdrv/charger.h>
#include <pbdrv/reset.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include <stm32f4xx_ll_spi.h>
#include <stm32f4xx_hal.h>

#include "py/mphal.h"
#include "py/runtime.h"

#include "lfs.h"

#include <pybricks/util_pb/pb_flash.h>

#if PYBRICKS_HUB_PRIMEHUB
#define FLASH_SIZE_TOTAL (32 * 0x100000)
#else
#define FLASH_SIZE_TOTAL (4 * 0x100000)
#endif

#define FLASH_SIZE_BOOT  (0x100000)
#define FLASH_SIZE_USER  (FLASH_SIZE_TOTAL - FLASH_SIZE_BOOT)
#define FLASH_SIZE_ERASE  (0x1000)
#define FLASH_SIZE_SIZE (4)
#define FLASH_SIZE_WRITE (256)

#define FLASH_TIMEOUT (100)

enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_GET_ID = 0x9F,
    #if PYBRICKS_HUB_PRIMEHUB
    FLASH_CMD_READ_DATA = 0x13,
    FLASH_CMD_ERASE_BLOCK = 0x21,
    FLASH_CMD_WRITE_DATA = 0x12,
    #else
    FLASH_CMD_READ_DATA = 0x03,
    FLASH_CMD_ERASE_BLOCK = 0x20,
    FLASH_CMD_WRITE_DATA = 0x02,
    #endif
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

    #if PYBRICKS_HUB_PRIMEHUB
    const uint8_t valid_id[] = {0xEF, 0x40, 0x19};
    #else
    const uint8_t valid_id[] = {0xEF, 0x40, 0x16};
    #endif

    // Verify flash device ID
    if (memcmp(valid_id, id_data, sizeof(valid_id))) {
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
        flash_enable(false);
        return err;
    }

    // Receive data
    err = HAL_SPI_Receive(&hspi2, status, 1, FLASH_TIMEOUT);
    flash_enable(false);
    return err;
}

static HAL_StatusTypeDef flash_wait_ready(void) {
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
    }
    return HAL_OK;
}

static HAL_StatusTypeDef flash_write_enable(void) {
    uint8_t command = FLASH_CMD_WRITE_ENABLE;
    flash_enable(true);
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, &command, sizeof(command), FLASH_TIMEOUT);
    flash_enable(false);
    return err;
}

static HAL_StatusTypeDef flash_send_address_command(uint8_t command, uint32_t address, bool keep_flash_enabled) {

    // Can only read/write user partition
    if (address > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }

    // Pack command and address as big endian. This is the absolute address,
    // starting in the boot partition.
    #if PYBRICKS_HUB_PRIMEHUB
    uint8_t data[5] = {command};
    pbio_set_uint32_be(&data[1], address);
    #else
    uint8_t data[4];
    pbio_set_uint32_be(&data[0], address);
    data[0] = command;
    #endif

    // Enable flash
    flash_enable(true);

    // Send address command
    HAL_StatusTypeDef err = HAL_SPI_Transmit(&hspi2, data, sizeof(data), FLASH_TIMEOUT);
    if (err != HAL_OK) {
        flash_enable(false);
        return err;
    }

    // On success, keep flash enabled if we are asked to
    if (!keep_flash_enabled) {
        flash_enable(false);
    }

    return HAL_OK;
}

static HAL_StatusTypeDef flash_raw_read(uint32_t address, uint8_t *buffer, uint32_t size) {

    // Enable flash and send the read command with address, offset by boot partition
    HAL_StatusTypeDef err = flash_send_address_command(FLASH_CMD_READ_DATA, address, true);
    if (err != HAL_OK) {
        flash_enable(false);
        return err;
    }

    // Receive data
    err = HAL_SPI_Receive(&hspi2, buffer, size, FLASH_TIMEOUT);
    flash_enable(false);
    return err;
}


static HAL_StatusTypeDef flash_raw_write(uint32_t address, const uint8_t *buffer, uint32_t size) {

    // Enable write mode
    HAL_StatusTypeDef err = flash_write_enable();
    if (err != HAL_OK) {
        return err;
    }

    // Enable flash and send the write command with address, offset by boot partition
    err = flash_send_address_command(FLASH_CMD_WRITE_DATA, address, true);
    if (err != HAL_OK) {
        flash_enable(false);
        return err;
    }

    // Write the data
    err = HAL_SPI_Transmit(&hspi2, (uint8_t *)buffer, size, FLASH_TIMEOUT);
    flash_enable(false);
    if (err != HAL_OK) {
        return err;
    }
    return flash_wait_ready();
}

static HAL_StatusTypeDef flash_raw_block_erase(uint32_t address) {

    // Enable write mode
    HAL_StatusTypeDef err = flash_write_enable();
    if (err != HAL_OK) {
        return err;
    }

    // Enable flash and send the erase block command with address, offset by boot partition
    err = flash_send_address_command(FLASH_CMD_ERASE_BLOCK, address, false);
    if (err != HAL_OK) {
        return err;
    }
    return flash_wait_ready();
}


static HAL_StatusTypeDef flash_user_read(uint32_t address, uint8_t *buffer, uint32_t size) {
    if (address + size > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }
    return flash_raw_read(address + FLASH_SIZE_BOOT, buffer, size);
}

static HAL_StatusTypeDef flash_user_write(uint32_t address, const uint8_t *buffer, uint32_t size) {
    if (address + size > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }
    return flash_raw_write(address + FLASH_SIZE_BOOT, buffer, size);
}

static HAL_StatusTypeDef flash_user_block_erase(uint32_t address) {
    if (address > FLASH_SIZE_USER) {
        return HAL_ERROR;
    }
    return flash_raw_block_erase(address + FLASH_SIZE_BOOT);
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
        if (flash_user_read(block * c->block_size + off + done, buffer + done, read_now) != HAL_OK) {
            return LFS_ERR_IO;
        }
        done += read_now;

        // Give MicroPython and PBIO some time.
        MICROPY_EVENT_POLL_HOOK;
    }
    return LFS_ERR_OK;
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
        if (flash_user_write(block * c->block_size + off + done, buffer + done, write_now) != HAL_OK) {
            return LFS_ERR_IO;
        }
        done += write_now;

        // Give MicroPython and PBIO some time.
        MICROPY_EVENT_POLL_HOOK;
    }

    return LFS_ERR_OK;
}

static int block_device_erase(const struct lfs_config *c, lfs_block_t block) {
    // Erase block of flash. Block size assumed to match erase size.

    if (flash_user_block_erase(block * c->block_size) != HAL_OK) {
        return LFS_ERR_IO;
    }

    // Give MicroPython and PBIO some time.
    MICROPY_EVENT_POLL_HOOK;

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
    .block_size = FLASH_SIZE_ERASE,
    .block_count = FLASH_SIZE_USER / FLASH_SIZE_ERASE,
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
    if (flash_user_read(0, lfs_data, sizeof(lfs_data)) != HAL_OK) {
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

    if (flash_raw_read(address, buf, size) != HAL_OK) {
        return PBIO_ERROR_FAILED;
    }
    return PBIO_SUCCESS;
}

int lfs_file_open_retry(const char *path, int flags) {
    // Try to open the file
    int err = lfs_file_open(&lfs, &file, path, flags);

    // If file is already open, close it and retry
    if (err == LFS_ERR_NOMEM) {
        err = lfs_file_close(&lfs, &file);
        if (err != LFS_ERR_OK) {
            return err;
        }

        err = lfs_file_open(&lfs, &file, path, flags);
    }
    return err;
}

pbio_error_t pb_flash_file_open_get_size(const char *path, uint32_t *size) {

    // Check that flash was initialized
    if (!flash_initialized) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Open file
    if (lfs_file_open_retry(path, LFS_O_RDONLY) != LFS_ERR_OK) {
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
    if (lfs_file_open_retry(path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
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

extern const mp_obj_module_t mp_module_ujson;

pbio_error_t pb_flash_restore_firmware(void) {
    // Check that flash was initialized
    if (!flash_initialized) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Check that usb is plugged in.
    pbdrv_charger_status_t status = pbdrv_charger_get_status();
    if (status != PBDRV_CHARGER_STATUS_CHARGE && status != PBDRV_CHARGER_STATUS_COMPLETE) {
        mp_printf(&mp_plat_print, "Please connect the hub via USB and try again.\n");
        return PBIO_SUCCESS;
    }

    mp_printf(&mp_plat_print, "Checking firmware backup files.\n");

    // Open meta file
    if (lfs_file_open_retry("_firmware/lego-firmware.metadata.json", LFS_O_RDONLY) != LFS_ERR_OK) {
        mp_printf(&mp_plat_print, "Unable to open backup meta data file.\n");
        return PBIO_ERROR_FAILED;
    }
    // Read meta file
    size_t meta_size = file.size;
    char *meta_data = m_new(char, meta_size);
    if (lfs_file_read(&lfs, &file, meta_data, file.size) != file.size) {
        mp_printf(&mp_plat_print, "Unable to read backup meta data file.\n");
        return PBIO_ERROR_FAILED;
    }

    // Close meta file
    if (lfs_file_close(&lfs, &file) != LFS_ERR_OK) {
        mp_printf(&mp_plat_print, "Unable to close backup meta data file.\n");
        return PBIO_ERROR_FAILED;
    }

    // Read meta data into MicroPython dictionary.
    mp_obj_t loads_func = mp_obj_dict_get(mp_module_ujson.globals, MP_ROM_QSTR(MP_QSTR_loads));
    mp_obj_t meta_dict = mp_call_function_1(loads_func, mp_obj_new_str(meta_data, meta_size));

    // Get relevant meta data
    mp_int_t meta_firmware_size = mp_obj_get_int(mp_obj_dict_get(meta_dict, MP_OBJ_NEW_QSTR(qstr_from_str("firmware-size"))));
    mp_int_t meta_device_id = mp_obj_get_int(mp_obj_dict_get(meta_dict, MP_OBJ_NEW_QSTR(qstr_from_str("device-id"))));
    mp_obj_t meta_firmware_version = mp_obj_dict_get(meta_dict, MP_OBJ_NEW_QSTR(qstr_from_str("firmware-version")));

    mp_printf(&mp_plat_print, "Detected firmware backup: ");
    mp_obj_print(meta_firmware_version, PRINT_STR);
    mp_printf(&mp_plat_print, " (%d) bytes.\n", meta_firmware_size);

    // TODO: Get from platform data or hub type
    #if PYBRICKS_HUB_PRIMEHUB
    mp_int_t valid_device_id = 0x81;
    #else
    mp_int_t valid_device_id = 0x83;
    #endif

    // Verify meta data
    if (meta_device_id != valid_device_id) {
        mp_printf(&mp_plat_print, "The firmware backup is not valid for this hub.\n");
        return PBIO_ERROR_FAILED;
    }

    mp_printf(&mp_plat_print, "Preparing storage for firmware installation...\n");

    HAL_StatusTypeDef err;

    // Erase boot partition
    for (uint32_t address = 0; address < FLASH_SIZE_BOOT; address += FLASH_SIZE_ERASE) {
        err = flash_raw_block_erase(address);
        if (err != HAL_OK) {
            mp_printf(&mp_plat_print, "Unable to erase storage.\n");
            return PBIO_ERROR_IO;
        }
        MICROPY_EVENT_POLL_HOOK;
    }

    // Check that usb is plugged in.
    status = pbdrv_charger_get_status();
    if (status != PBDRV_CHARGER_STATUS_CHARGE && status != PBDRV_CHARGER_STATUS_COMPLETE) {
        mp_printf(&mp_plat_print, "Please connect the hub via USB and try again.\n");
        return PBIO_SUCCESS;
    }

    mp_printf(&mp_plat_print, "Restoring firmware...\n");

    // Open firmware file
    if (lfs_file_open_retry("_firmware/lego-firmware.bin", LFS_O_RDONLY) != LFS_ERR_OK) {
        mp_printf(&mp_plat_print, "Unable to open backup firmware file.\n");
        return PBIO_ERROR_FAILED;
    }

    // All known back up firmwares are much larger than this.
    if (file.size < 128 * 1024 || file.size != meta_firmware_size) {
        // TODO: Check that sha256 matches metadata
        mp_printf(&mp_plat_print, "Invalid backup firmware file.\n");
        return PBIO_ERROR_FAILED;
    }

    // Initialize buffers
    lfs_size_t done = 0;
    lfs_size_t read_size, write_size;
    uint8_t buf[FLASH_SIZE_WRITE];
    uint8_t *buf_start;

    // Read/write the firmware page by page
    while (done < file.size) {

        // The first page is a special case
        if (done == 0) {
            // It starts of with the firmware size
            pbio_set_uint32_le(buf, file.size);

            // We read just enough to fill up the page
            buf_start = buf + FLASH_SIZE_SIZE;
            read_size = FLASH_SIZE_WRITE - FLASH_SIZE_SIZE;

            // We still write a full page
            write_size = FLASH_SIZE_WRITE;
        }
        // Otherwise, we just look at how much is remaining
        else {
            // Take the remaining size, or at most one page
            read_size = file.size - done;
            if (read_size > sizeof(buf)) {
                read_size = sizeof(buf);
            }
            // We write as much as we read
            write_size = read_size;
            buf_start = buf;
        }

        // Read data from the firmware backup file
        if (lfs_file_read(&lfs, &file, buf_start, read_size) != read_size) {
            mp_printf(&mp_plat_print, "Unable to read backup firmware file.\n");
            return PBIO_ERROR_FAILED;
        }

        // Write the data
        err = flash_raw_write(done == 0 ? 0 : done + 4, buf, write_size);
        if (err != HAL_OK) {
            mp_printf(&mp_plat_print, "Unable to copy the backup firmware file.\n");
            return PBIO_ERROR_IO;
        }

        done += read_size;
    }

    // Close firmware file
    if (lfs_file_close(&lfs, &file) != LFS_ERR_OK) {
        mp_printf(&mp_plat_print, "Unable to close the backup firmware file.\n");
        return PBIO_ERROR_FAILED;
    }

    mp_printf(&mp_plat_print, "Done. The hub will now reboot. Please keep USB attached.\n");
    mp_hal_delay_ms(500);
    pbdrv_reset(PBDRV_RESET_ACTION_RESET);

    return PBIO_SUCCESS;
}

#endif // (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)

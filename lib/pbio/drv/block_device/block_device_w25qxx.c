// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for W25Qxx.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_W25Q32 || PBDRV_CONFIG_BLOCK_DEVICE_W25Q256

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>

#include <pbdrv/spi.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include "./block_device_w25qxx.h"
#include "../core.h"
#include "../../src/processes.h"

PROCESS(pbdrv_block_device_w25qxx_process, "w25qxx");

typedef struct {
    /** Platform-specific data */
    const pbdrv_block_device_w25qxx_platform_data_t *pdata;
    /** The spi device */
    pbdrv_spi_dev_t *spi;
} pbdrv_block_device_drv_t;

/** The one block device instance. */
static pbdrv_block_device_drv_t bdev = {
    .pdata = &pbdrv_block_device_w25qxx_platform_data,
};

/** Flash status flags. */
enum {
    FLASH_STATUS_BUSY = 0x01,
    FLASH_STATUS_WRITE_ENABLED = 0x02,
};

// Select constant values based on device type.
#if PBDRV_CONFIG_BLOCK_DEVICE_W25Q32
#define W25Qxx(Q32, Q256) (Q32)
#elif PBDRV_CONFIG_BLOCK_DEVICE_W25Q256
#define W25Qxx(Q32, Q256) (Q256)
#endif

// 3 or 4 byte addressing mode.
#define FLASH_ADDRESS_SIZE (W25Qxx(3, 4))

// Flash commands.
enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_GET_ID = 0x9F,
    FLASH_CMD_READ_DATA = W25Qxx(0x03, 0x13),
    FLASH_CMD_ERASE_BLOCK = W25Qxx(0x20, 0x21),
    FLASH_CMD_WRITE_DATA = W25Qxx(0x02, 0x12),
};

// Flash sizes.
enum {
    FLASH_SIZE_ERASE = 4 * 1024,
    FLASH_SIZE_WRITE = 256,
};

// W25Q32 Device and manufacturer ID.
static const uint8_t device_id[] = {0xEF, 0x40, W25Qxx(0x16, 0x19)};

/**
 * Read a chunk of data from flash.
 */
PT_THREAD(block_device_read_thread(struct pt *pt, uint32_t address, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {

    static struct pt spi_pt;
    static struct timer pause;

    // SPI command for sending flash address
    static uint8_t address_bytes[FLASH_ADDRESS_SIZE + 1];
    static const pbdrv_spi_command_t cmd_addr = {
        .operation = PBDRV_SPI_WRITE,
        .buffer = address_bytes,
        .size = sizeof(address_bytes),
    };

    // SPI command to receive data. Buffer and size set below.
    static pbdrv_spi_command_t cmd_data = {
        .operation = PBDRV_SPI_READ_AND_DISABLE,
    };

    // static struct etimer pause_timer;

    PT_BEGIN(pt);

    // Pack address as big endian, with command byte at the start.
    pbio_set_uint32_be(&cmd_addr.buffer[FLASH_ADDRESS_SIZE - 3], address);
    cmd_addr.buffer[0] = FLASH_CMD_READ_DATA;

    // Write address command.
    PT_SPAWN(pt, &spi_pt, pbdrv_spi_command_thread(&spi_pt, bdev.spi, &cmd_addr, err));
    if (*err != PBIO_SUCCESS) {
        PT_EXIT(pt);
    }

    // Revisit: This shouldn't be needed. But if we don't pause here, we
    // sometimes miss one byte in the next step on essential hub, causing the
    // spi to time out.
    timer_set(&pause, 5);
    while (!timer_expired(&pause)) {
        process_poll(&pbdrv_block_device_w25qxx_process);
        PT_YIELD(pt);
    }

    // Receive the data.
    cmd_data.buffer = buffer;
    cmd_data.size = size;
    PT_SPAWN(pt, &spi_pt, pbdrv_spi_command_thread(&spi_pt, bdev.spi, &cmd_data, err));

    // This is the last operation, so we don't need to check the final error.
    PT_END(pt);
}

// SPI command to enable flash writing.
static const pbdrv_spi_command_t cmd_write_enable = {
    .operation = PBDRV_SPI_WRITE_AND_DISABLE,
    .buffer = &(uint8_t) {FLASH_CMD_WRITE_ENABLE},
    .size = 1,
};

// SPI command to read the write-status byte.
static const pbdrv_spi_command_t cmd_status_tx = {
    .operation = PBDRV_SPI_WRITE,
    .buffer = &(uint8_t) {FLASH_CMD_GET_STATUS},
    .size = 1,
};

/**
 * Write or erase a chunk of data from flash.
 *
 * In case of write, address must be aligned with a page.
 *
 * In case of erase (indicated by size = 0), address must be aligned with a sector.
 *
 */
static PT_THREAD(flash_write_thread(struct pt *pt, uint32_t address, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {

    static struct pt spi_pt;

    // SPI command for sending flash address. Operation type
    // is different for write and erase command, so set later.
    static uint8_t address_bytes[FLASH_ADDRESS_SIZE + 1];
    static pbdrv_spi_command_t cmd_addr = {
        .buffer = address_bytes,
        .size = sizeof(address_bytes),
    };

    // SPI command to receive the write-status response.
    static uint8_t status;
    static const pbdrv_spi_command_t cmd_status_rx = {
        .operation = PBDRV_SPI_READ_AND_DISABLE,
        .buffer = &status,
        .size = sizeof(status),
    };

    // SPI command to transmit data. Buffer and size set below.
    static pbdrv_spi_command_t cmd_data = {
        .operation = PBDRV_SPI_WRITE_AND_DISABLE,
    };

    PT_BEGIN(pt);

    // Enable write mode.
    PT_SPAWN(pt, &spi_pt, pbdrv_spi_command_thread(&spi_pt, bdev.spi, &cmd_write_enable, err));
    if (*err != PBIO_SUCCESS) {
        PT_EXIT(pt);
    }

    // Pack address as big endian, with command byte at the start.
    pbio_set_uint32_be(&address_bytes[FLASH_ADDRESS_SIZE - 3], address);
    address_bytes[0] = size == 0 ? FLASH_CMD_ERASE_BLOCK : FLASH_CMD_WRITE_DATA;

    // Write the address command. In case of write, don't disable SPI.
    // It needs to remain enabled to send data in the next step.
    cmd_addr.operation = size == 0 ? PBDRV_SPI_WRITE_AND_DISABLE : PBDRV_SPI_WRITE;
    PT_SPAWN(pt, &spi_pt, pbdrv_spi_command_thread(&spi_pt, bdev.spi, &cmd_addr, err));
    if (*err != PBIO_SUCCESS) {
        PT_EXIT(pt);
    }

    // Write the data, or skip in case of erase.
    if (size != 0) {
        cmd_data.buffer = buffer;
        cmd_data.size = size;
        PT_SPAWN(pt, &spi_pt, pbdrv_spi_command_thread(&spi_pt, bdev.spi, &cmd_data, err));
        if (*err != PBIO_SUCCESS) {
            PT_EXIT(pt);
        }
    }

    // Wait for busy flag to clear.
    do {
        // Send command to read status.
        PT_SPAWN(pt, &spi_pt, pbdrv_spi_command_thread(&spi_pt, bdev.spi, &cmd_status_tx, err));
        if (*err != PBIO_SUCCESS) {
            PT_EXIT(pt);
        }

        // Read the status.
        PT_SPAWN(pt, &spi_pt, pbdrv_spi_command_thread(&spi_pt, bdev.spi, &cmd_status_rx, err));
        if (*err != PBIO_SUCCESS) {
            PT_EXIT(pt);
        }
    } while (status & (FLASH_STATUS_BUSY | FLASH_STATUS_WRITE_ENABLED));

    // The task is ready.
    PT_END(pt);
}

// Command to trigger device ID read.
static const pbdrv_spi_command_t cmd_id_tx = {
    .operation = PBDRV_SPI_WRITE,
    .buffer = &(uint8_t) {FLASH_CMD_GET_ID},
    .size = 1,
};

/**
 * Erases then writes a chunk of data to flash.
 *
 * Address must be sector aligned.
 */
PT_THREAD(block_device_erase_and_write_thread(struct pt *pt, uint32_t address, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {

    static struct pt bdev_pt;
    static uint32_t offset;
    static uint32_t size_now;
    static uint32_t remaining;

    PT_BEGIN(pt);

    // Erase sector by sector.
    for (offset = 0; offset < size; offset += FLASH_SIZE_ERASE) {
        // Writing size 0 means erase.
        PT_SPAWN(pt, &bdev_pt, flash_write_thread(&bdev_pt, address + offset, NULL, 0, err));
        if (*err != PBIO_SUCCESS) {
            PT_EXIT(pt);
        }
    }

    // Write page by page.
    remaining = size;
    while (remaining) {
        size_now = remaining < FLASH_SIZE_WRITE ? remaining : FLASH_SIZE_WRITE;
        PT_SPAWN(pt, &bdev_pt, flash_write_thread(&bdev_pt, address + size - remaining, buffer + size - remaining, size_now, err));
        if (*err != PBIO_SUCCESS) {
            PT_EXIT(pt);
        }
        remaining -= size_now;
    }
    PT_END(pt);
}

void pbdrv_block_device_init(void) {
    pbdrv_init_busy_up();
    process_start(&pbdrv_block_device_w25qxx_process);
}

void pbdrv_block_device_deinit(void) {
    pbdrv_init_busy_up();
    process_post(&pbdrv_block_device_w25qxx_process, PROCESS_EVENT_CONTINUE, NULL);
}

static void pbdrv_block_device_hook(void) {
    process_poll(&pbdrv_block_device_w25qxx_process);
}

extern uint8_t _app_data_ram_start[];
extern uint8_t _app_data_flash_start[];
extern uint8_t _app_data_flash_end[];

// Maximum size of stored user data.
#define APP_DATA_SIZE_MAX ((uint32_t)(_app_data_flash_end - _app_data_flash_start))

// Numeric representation of starting address.
#define FLASH_ADDRESS_NUM ((uint32_t)((uint32_t *)_app_data_flash_start))

PROCESS_THREAD(pbdrv_block_device_w25qxx_process, ev, data) {

    static struct pt pt;
    static pbio_error_t err;
    static uint32_t read_size;
    static uint32_t read_done;
    static uint32_t write_size;

    // Command to read ID data.
    static uint8_t id_data[sizeof(device_id)];
    static const pbdrv_spi_command_t cmd_id_rx = {
        .operation = PBDRV_SPI_READ_AND_DISABLE,
        .buffer = id_data,
        .size = sizeof(id_data),
    };

    PROCESS_BEGIN();

    // Get SPI device.
    while (pbdrv_spi_get(bdev.pdata->spi_id, &bdev.spi) != PBIO_SUCCESS) {
        PROCESS_PAUSE();
    }
    pbdrv_spi_set_hook(bdev.spi, pbdrv_block_device_hook);

    // Write the ID getter command
    PROCESS_PT_SPAWN(&pt, pbdrv_spi_command_thread(&pt, bdev.spi, &cmd_id_tx, &err));
    if (err != PBIO_SUCCESS) {
        PROCESS_EXIT();
    }

    // Get ID command reply
    PROCESS_PT_SPAWN(&pt, pbdrv_spi_command_thread(&pt, bdev.spi, &cmd_id_rx, &err));
    if (err != PBIO_SUCCESS) {
        PROCESS_EXIT();
    }

    // Verify flash device ID
    if (memcmp(device_id, id_data, sizeof(id_data))) {
        PROCESS_EXIT();
    }

    // Read available data size.
    PROCESS_PT_SPAWN(&pt, block_device_read_thread(&pt,
        FLASH_ADDRESS_NUM, _app_data_ram_start, sizeof(uint32_t), &err));

    // Preload user data to RAM.
    // REVISIT: This is supposed to work in one go. Do in chunks for now.
    read_size = pbio_get_uint32_le(_app_data_ram_start);
    read_done = 0;
    while (read_size > 0 && read_size < APP_DATA_SIZE_MAX && read_done < read_size) {
        PROCESS_PT_SPAWN(&pt, block_device_read_thread(&pt,
            FLASH_ADDRESS_NUM + read_done, _app_data_ram_start + read_done, 2048, &err));
        read_done += 2048;
    }

    // The first word represents the size of the data that should be written
    // back on shutdown, so reset it.
    pbio_set_uint32_le(_app_data_ram_start, 0);

    // Initialization done.
    pbdrv_init_busy_down();

    // Wait for signal from deinit.
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);

    // Write the user data back to flash, if any.
    write_size = pbio_get_uint32_le(_app_data_ram_start);
    if (write_size > 0 && write_size <= APP_DATA_SIZE_MAX) {
        PROCESS_PT_SPAWN(&pt, block_device_erase_and_write_thread(&pt,
            FLASH_ADDRESS_NUM, _app_data_ram_start, write_size, &err));
    }

    // Deinitialization done.
    pbdrv_init_busy_down();

    PROCESS_END();
}


#endif // PBDRV_CONFIG_BLOCK_DEVICE_W25Q32 || PBDRV_CONFIG_BLOCK_DEVICE_W25Q256

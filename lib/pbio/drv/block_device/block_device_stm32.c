// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for STM32.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_STM32F0 || PBDRV_CONFIG_BLOCK_DEVICE_STM32L4

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <pbio/util.h>

#include STM32_HAL_H

extern uint8_t _app_data_ram_start[];
extern uint8_t _app_data_flash_start[];
extern uint8_t _app_data_flash_end[];

#define APP_DATA_SIZE_MAX ((uint32_t)(_app_data_flash_end - _app_data_flash_start))

void pbdrv_block_device_init(void) {

    // Preload user data to RAM
    memcpy(_app_data_ram_start, _app_data_flash_start, APP_DATA_SIZE_MAX);

    // The first word represents the size of the data that should be written
    // back on shutdown, so reset it.
    pbio_set_uint32_le(_app_data_ram_start, 0);
}

typedef union _double_word_t {
    uint8_t data[8];
    uint64_t dword;
} double_word_t;

void pbdrv_block_device_deinit(void) {

    // Exit right away if there is no (valid) data to write at all.
    uint32_t write_size = pbio_get_uint32_le(_app_data_ram_start);
    if (write_size == 0 || write_size > APP_DATA_SIZE_MAX) {
        return;
    }

    // Unlock flash for writing.
    HAL_StatusTypeDef err = HAL_FLASH_Unlock();
    if (err != HAL_OK) {
        return;
    }

    // The HAL API requires a numeric offset value instead of an address.
    uint32_t base = (uint32_t)((uint32_t *)_app_data_flash_start);

    // Erase only as much as we need.
    FLASH_EraseInitTypeDef erase_init = {
        #if PBDRV_CONFIG_BLOCK_DEVICE_STM32F0
        .PageAddress = base,
        #else
        .Banks = FLASH_BANK_1,
        .Page = (FLASH_SIZE - (APP_DATA_SIZE_MAX)) / FLASH_PAGE_SIZE,
        #endif
        .NbPages = write_size / FLASH_PAGE_SIZE + (write_size % FLASH_PAGE_SIZE != 0),
        .TypeErase = FLASH_TYPEERASE_PAGES
    };
    uint32_t page_error;
    err = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (err != HAL_OK || page_error != 0xFFFFFFFFU) {
        HAL_FLASH_Lock();
        return;
    }

    // Write data chunk by chunk.
    uint32_t done = 0;
    while (done < write_size) {

        // Adjust write type for last page.
        uint32_t type = FLASH_TYPEPROGRAM_DOUBLEWORD;

        // Cast data to a doubleword.
        double_word_t *data = (double_word_t *)(_app_data_ram_start + done);

        // Disable interrupts while writing.
        uint32_t state = __get_PRIMASK();
        __disable_irq();

        // Write the data and re-enable interrupts.
        err = HAL_FLASH_Program(type, base + done, data->dword);
        __set_PRIMASK(state);
        if (err != HAL_OK) {
            break;
        }

        // Update write progress.
        done += sizeof(double_word_t);
    }

    // Lock flash on completion or failure.
    HAL_FLASH_Lock();
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_STM32F0 || PBDRV_CONFIG_BLOCK_DEVICE_STM32L4

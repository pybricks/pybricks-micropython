// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for internal flash on STM32 boards.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32

#include <stdint.h>
#include <string.h>

#include <contiki.h>

#include "../core.h"

#include <pbdrv/block_device.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include STM32_HAL_H

uint32_t pbdrv_block_device_get_size(void) {
    // Defined in linker script.
    extern uint32_t _pbdrv_block_device_storage_size;
    return (uint32_t)(&_pbdrv_block_device_storage_size);
}

void pbdrv_block_device_init(void) {
}

static void (*_callback)(void);
void pbdrv_block_device_set_callback(void (*callback)(void)) {
    _callback = callback;
}

extern uint8_t _pbdrv_block_device_storage_start[];

PT_THREAD(pbdrv_block_device_read(struct pt *pt, uint32_t offset, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {

    PT_BEGIN(pt);

    // Exit on invalid size.
    if (size == 0 || offset + size > pbdrv_block_device_get_size()) {
        *err = PBIO_ERROR_INVALID_ARG;
        PT_EXIT(pt);
    }

    // Copy requested data to RAM.
    memcpy(buffer, _pbdrv_block_device_storage_start + offset, size);
    *err = PBIO_SUCCESS;

    PT_END(pt);
}

typedef union _double_word_t {
    uint8_t data[8];
    uint64_t dword;
} double_word_t;

static pbio_error_t block_device_erase_and_write(uint8_t *buffer, uint32_t size) {

    static const uint32_t base_address = (uint32_t)(&_pbdrv_block_device_storage_start[0]);

    // Exit on invalid size.
    if (size == 0 || size > pbdrv_block_device_get_size()) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Unlock flash for writing.
    HAL_StatusTypeDef hal_err = HAL_FLASH_Unlock();
    if (hal_err != HAL_OK) {
        return PBIO_ERROR_IO;
    }

    // Erase only as much as we need.
    FLASH_EraseInitTypeDef erase_init = {
        #if defined(STM32F0)
        .PageAddress = base_address,
        #elif defined(STM32L4)
        .Banks = FLASH_BANK_1, // Hard coded for STM32L431RC.
        .Page = (FLASH_SIZE - (pbdrv_block_device_get_size())) / FLASH_PAGE_SIZE,
        #else
        #error "Unsupported target."
        #endif
        .NbPages = (size + (FLASH_PAGE_SIZE - 1)) / FLASH_PAGE_SIZE,
        .TypeErase = FLASH_TYPEERASE_PAGES
    };

    // Disable interrupts to avoid crash if reading while writing/erasing.
    uint32_t state = __get_PRIMASK();
    __disable_irq();

    // Erase and re-enable interrupts.
    uint32_t page_error;
    hal_err = HAL_FLASHEx_Erase(&erase_init, &page_error);
    __set_PRIMASK(state);
    if (hal_err != HAL_OK || page_error != 0xFFFFFFFFU) {
        HAL_FLASH_Lock();
        return PBIO_ERROR_IO;
    }

    // Write data chunk by chunk.
    uint32_t done = 0;
    while (done < size) {

        // Disable interrupts while writing as above.
        state = __get_PRIMASK();
        __disable_irq();

        // Write the data and re-enable interrupts.
        hal_err = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, base_address + done, *(uint64_t *)(buffer + done));
        __set_PRIMASK(state);
        if (hal_err != HAL_OK) {
            HAL_FLASH_Lock();
            return PBIO_ERROR_IO;
        }

        // Update write progress.
        done += sizeof(double_word_t);
    }

    // Lock flash on completion.
    HAL_FLASH_Lock();

    return PBIO_SUCCESS;
}

PT_THREAD(pbdrv_block_device_store(struct pt *pt, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {
    PT_BEGIN(pt);
    *err = block_device_erase_and_write(buffer, size);
    PT_END(pt);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32

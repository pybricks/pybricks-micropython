// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for internal flash on STM32 boards.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32

#include <stdint.h>
#include <string.h>

#include "../core.h"
#include "../sys/storage_data.h"

#include <pbdrv/block_device.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include STM32_HAL_H

extern uint8_t _pbdrv_block_device_storage_start[];

static struct {
    /**
     * How much data to write on shutdown and load on the next boot. Includes
     * the size of this field, because it is also saved.
     */
    uint32_t saved_size;
    /**
     * Checksum complement to satisfy bootloader requirements. This ensures
     * that words in the scanned area still add up to precisely 0 after user
     * data was written. Needs to be volatile as it gets optimized out when it
     * is never read by our code.
     */
    volatile uint32_t checksum_complement;
    /**
     * A copy of the data loaded from flash and application heap. The first
     * portion of this, up to pbdrv_block_device_get_writable_size() bytes,
     * gets saved to flash at shutdown.
     */
    union {
        // ensure that data is properly aligned for pbsys_storage_data_map_t
        pbsys_storage_data_map_t data_map;
        uint8_t data[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
    };
} ramdisk __attribute__((section(".noinit"), used));

const uint32_t header_size = sizeof(ramdisk.saved_size) + sizeof(ramdisk.checksum_complement);

uint32_t pbdrv_block_device_get_writable_size(void) {
    return PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE - header_size;
}

static pbio_error_t init_err;

void pbdrv_block_device_init(void) {

    uint32_t size;

    memcpy(&size, _pbdrv_block_device_storage_start, sizeof(size));

    // Exit on invalid size.
    if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE) {
        // This error will be retrieved when higher level code requests the
        // ramdisk, so that it can reset data to firmware defaults.
        init_err = PBIO_ERROR_INVALID_ARG;
        return;
    }

    // Load requested amount of data to RAM. Also re-reads the size value.
    memcpy(&ramdisk, _pbdrv_block_device_storage_start, size);
}

pbio_error_t pbdrv_block_device_get_data(pbsys_storage_data_map_t **data) {
    *data = &ramdisk.data_map;
    return init_err;
}

// Updates checksum in data map to satisfy bootloader requirements.
static void pbdrv_block_device_update_ramdisk_size_and_checksum(uint32_t used_data_size) {

    // Total size includes used data and header.
    ramdisk.saved_size = used_data_size + header_size;

    // Align writable data by a double word, to simplify checksum
    // computation and storage drivers that write double words.
    while (ramdisk.saved_size % 8) {
        *((uint8_t *)&ramdisk + ramdisk.saved_size++) = 0;
    }

    // The area scanned by the bootloader adds up to 0 when all user data
    // is 0xFFFFFFFF. So the bootloader value up until just before the user
    // data is always 0 + the number of words in the scanned user data.
    extern uint32_t _pbsys_storage_checked_size;
    uint32_t checksize = (uint32_t)&_pbsys_storage_checked_size;
    uint32_t checksum = checksize / sizeof(uint32_t);

    // Don't count existing value.
    ramdisk.checksum_complement = 0;

    // Add checksum for each word in the written data and empty checked size.
    for (uint32_t offset = 0; offset < checksize; offset += sizeof(uint32_t)) {
        uint32_t *word = (uint32_t *)((uint8_t *)&ramdisk + offset);
        // Assume that everything after written data is erased by the block
        // device driver prior to writing.
        checksum += offset < ramdisk.saved_size ? *word : 0xFFFFFFFF;
    }

    // Set the checksum complement to cancel out user data checksum.
    ramdisk.checksum_complement = 0xFFFFFFFF - checksum + 1;
}

pbio_error_t pbdrv_block_device_write_all(pbio_os_state_t *state, uint32_t used_data_size) {

    // NB: This function is called as an awaitable for compatibility with other
    // external storage mediums. This implementation is blocking, but we only
    // use it during shutdown so this is acceptable.

    // Account for header size and make valid checksum.
    pbdrv_block_device_update_ramdisk_size_and_checksum(used_data_size);
    uint32_t size = ramdisk.saved_size;

    static const uint32_t base_address = (uint32_t)(&_pbdrv_block_device_storage_start[0]);

    // Exit if size is 0, too big, or not a multiple of double-word size.
    if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE || size % sizeof(uint64_t)) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Unlock flash for writing.
    HAL_StatusTypeDef hal_err = HAL_FLASH_Unlock();
    if (hal_err != HAL_OK) {
        return PBIO_ERROR_IO;
    }

    // Erase the whole user storage area.
    FLASH_EraseInitTypeDef erase_init = {
        #if defined(STM32F0)
        .PageAddress = base_address,
        #elif defined(STM32L4)
        .Banks = FLASH_BANK_1, // Hard coded for STM32L431RC.
        .Page = (FLASH_SIZE - (PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE)) / FLASH_PAGE_SIZE,
        #else
        #error "Unsupported target."
        #endif
        .NbPages = PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE / FLASH_PAGE_SIZE,
        .TypeErase = FLASH_TYPEERASE_PAGES
    };

    // Disable interrupts to avoid crash if reading while writing/erasing.
    uint32_t irq = __get_PRIMASK();
    __disable_irq();

    // Erase and re-enable interrupts.
    uint32_t page_error;
    hal_err = HAL_FLASHEx_Erase(&erase_init, &page_error);
    __set_PRIMASK(irq);
    if (hal_err != HAL_OK || page_error != 0xFFFFFFFFU) {
        HAL_FLASH_Lock();
        return PBIO_ERROR_IO;
    }

    // Write data chunk by chunk.
    uint32_t done = 0;
    while (done < size) {

        // Disable interrupts while writing as above.
        irq = __get_PRIMASK();
        __disable_irq();

        // Write the data and re-enable interrupts.
        hal_err = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, base_address + done, *(uint64_t *)((void *)&ramdisk + done));
        __set_PRIMASK(irq);
        if (hal_err != HAL_OK) {
            HAL_FLASH_Lock();
            return PBIO_ERROR_IO;
        }

        // Update write progress.
        done += sizeof(uint64_t);
    }

    // Lock flash on completion.
    HAL_FLASH_Lock();

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32

// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for Internal_flash.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32

#include <stdint.h>
#include <string.h>

#include <contiki.h>

#include "../core.h"

#include <pbdrv/block_device.h>

#include <pbio/error.h>
#include <pbio/util.h>

uint32_t pbdrv_block_device_get_size(void) {
    // Defined in linker script.
    extern uint32_t _pbdrv_block_device_storage_size;
    return (uint32_t)(&_pbdrv_block_device_storage_size);
}

void pbdrv_block_device_init(void) {
}

void pbdrv_block_device_set_callback(void (*callback)(void)) {
}

PT_THREAD(pbdrv_block_device_read(struct pt *pt, uint32_t offset, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {

    PT_BEGIN(pt);

    // TODO: Read from user flash. For now this just loads
    // the mpy file appended to the firmware to keep Pybricks Code working.
    extern uint32_t _pb_user_mpy_size;
    if (_pb_user_mpy_size + 8 > pbdrv_block_device_get_size()) {
        *err = PBIO_ERROR_FAILED;
        PT_EXIT(pt);
    }
    extern uint8_t _pb_user_mpy_data[];
    pbio_set_uint32_le(buffer, _pb_user_mpy_size + 8);
    pbio_set_uint32_le(&buffer[4], _pb_user_mpy_size);
    memcpy(&buffer[8], _pb_user_mpy_data, _pb_user_mpy_size);
    *err = PBIO_SUCCESS;

    PT_END(pt);
}

PT_THREAD(pbdrv_block_device_store(struct pt *pt, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {

    PT_BEGIN(pt);
    // TODO: Implement saving to user flash.
    *err = PBIO_ERROR_NOT_IMPLEMENTED;
    PT_END(pt);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32

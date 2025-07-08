// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup BlockDeviceDriver Driver: Block device.
 * @{
 */

#ifndef _PBDRV_BLOCK_DEVICE_H_
#define _PBDRV_BLOCK_DEVICE_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

#include <pbio/os.h>

#if PBDRV_CONFIG_BLOCK_DEVICE

/**
 * Gets the block device data.
 *
 * @param [out] disk    Pointer to the loaded data.
 * @return              ::PBIO_SUCCESS on success.
 *                      ::PBIO_ERROR_AGAIN if the RAM disk is still loading.
 *                      ::PBIO_ERROR_INVALID_ARG if the detected size was invalid.
 *                      ::PBIO_ERROR_IO (driver-specific error), though boot
 *                        does not proceed if this happens.
 */
pbio_error_t pbdrv_block_device_get_data(uint8_t **data);

/**
 * Writes the "RAM Disk" to storage. May erase entire disk prior to writing.
 *
 * On systems with data storage on an external chip, this is implemented with
 * non-blocking I/O operations. On systems where data is saved on internal
 * flash, this may be partially implemented with blocking operations, so this
 * function should only be used when this is permissible.
 *
 * @param [in] state    Protothread state.
 * @param [in] size     How many bytes to write.
 * @return              ::PBIO_SUCCESS on success.
 *                      ::PBIO_INVALID_ARGUMENT if size is too big.
 *                      ::PBIO_ERROR_BUSY (driver-specific error)
 *                      ::PBIO_ERROR_TIMEDOUT (driver-specific error)
 *                      ::PBIO_ERROR_IO (driver-specific error)
 */
pbio_error_t pbdrv_block_device_write_all(pbio_os_state_t *state, uint32_t used_data_size);

/**
 * Gets the maximum writable size for user data that can be saved to the device.
 *
 * @return   Size in bytes.
 */
uint32_t pbdrv_block_device_get_writable_size(void);

#else

static inline pbio_error_t pbdrv_block_device_get_data(uint8_t **data) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_block_device_write_all(pbio_os_state_t *state, uint32_t used_data_size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline uint32_t pbdrv_block_device_get_writable_size(void) {
    return 0;
}

#endif

#endif // _PBDRV_BLOCK_DEVICE_H_

/** @} */

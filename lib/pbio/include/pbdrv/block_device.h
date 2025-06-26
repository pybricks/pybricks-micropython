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
 * Read data from a storage device.
 *
 * On systems with data storage on an external chip, this is implemented with
 * non-blocking I/O operations.
 *
 * On systems where data is memory accessible, this may be implemented with
 * a single (blocking) memcpy operation.
 *
 * @param [in] state    Protothread state.
 * @param [in] offset   Offset from the base address for this block device.
 * @param [in] buffer   Buffer to read the data into.
 * @param [in] size     How many bytes to read.
 * @return              ::PBIO_SUCCESS on success.
 *                      ::PBIO_INVALID_ARGUMENT if offset + size is too big.
 *                      ::PBIO_ERROR_BUSY (driver-specific error)
 *                      ::PBIO_ERROR_TIMEDOUT (driver-specific error)
 *                      ::PBIO_ERROR_IO (driver-specific error)
 */
pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size);

/**
 * Store data on storage device, starting from the base address.
 *
 * This erases as many sectors as needed for the given size prior to writing.
 *
 * On systems with data storage on an external chip, this is implemented with
 * non-blocking I/O operations.
 *
 * On systems where data is saved on internal flash, this may be partially
 * implemented with blocking operations, so this function should only be used
 * when this is permissible.
 *
 * @param [in] state    Protothread state.
 * @param [in] buffer   Data buffer to write.
 * @param [in] size     How many bytes to write.
 * @return              ::PBIO_SUCCESS on success.
 *                      ::PBIO_INVALID_ARGUMENT if size is too big.
 *                      ::PBIO_ERROR_BUSY (driver-specific error)
 *                      ::PBIO_ERROR_TIMEDOUT (driver-specific error)
 *                      ::PBIO_ERROR_IO (driver-specific error)
 */
pbio_error_t pbdrv_block_device_store(pbio_os_state_t *state, uint8_t *buffer, uint32_t size);

#else

static inline pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_block_device_store(pbio_os_state_t *state, uint8_t *buffer, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif // _PBDRV_BLOCK_DEVICE_H_

/** @} */

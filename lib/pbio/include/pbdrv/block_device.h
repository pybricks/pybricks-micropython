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

#include <contiki.h>

#if PBDRV_CONFIG_BLOCK_DEVICE

/**
 * Gets the size of the block device.
 *
 * @return              The maximum size that can be read or written.
 */
uint32_t pbdrv_block_device_get_size(void);

/**
 * Read data from a storage device.
 *
 * On systems with data storage on an external chip, this is implemented with
 * non-blocking I/O operations.
 *
 * On systems where data is memory accessible, this may be implemented with
 * a single (blocking) memcpy operation.
 *
 * @param [in] pt       Protothread to run this function in.
 * @param [in] offset   Offset from the base address for this block device.
 * @param [in] buffer   Buffer to read the data into.
 * @param [in] size     How many bytes to read.
 * @param [out] err     ::PBIO_SUCCESS on success.
 *                      ::PBIO_INVALID_ARGUMENT if offset + size is too big.
 *                      ::PBIO_ERROR_BUSY (driver-specific error)
 *                      ::PBIO_ERROR_TIMEDOUT (driver-specific error)
 *                      ::PBIO_ERROR_IO (driver-specific error)
 */
PT_THREAD(pbdrv_block_device_read(struct pt *pt, uint32_t offset, uint8_t *buffer, uint32_t size, pbio_error_t *err));

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
 * @param [in] pt       Protothread to run this function in.
 * @param [in] buffer   Data buffer to write.
 * @param [in] size     How many bytes to write.
 * @param [out] err     ::PBIO_SUCCESS on success.
 *                      ::PBIO_INVALID_ARGUMENT if size is too big.
 *                      ::PBIO_ERROR_BUSY (driver-specific error)
 *                      ::PBIO_ERROR_TIMEDOUT (driver-specific error)
 *                      ::PBIO_ERROR_IO (driver-specific error)
 */
PT_THREAD(pbdrv_block_device_store(struct pt *pt, uint8_t *buffer, uint32_t size, pbio_error_t *err));

/**
 * Sets the callback function to inform caller of block device events.
 *
 * This is normally used to poll the process that spawns the above asynchronous
 * functions, so that it can continue.
 *
 * @param [in] callback  Callback function.
 */
void pbdrv_block_device_set_callback(void (*callback)(void));

#else

static inline uint32_t pbdrv_block_device_get_size(void) {
    return 0;
}
static inline PT_THREAD(pbdrv_block_device_read(struct pt *pt, uint32_t offset, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {
    PT_BEGIN(pt);
    *err = PBIO_ERROR_NOT_SUPPORTED;
    PT_END(pt);
}
static inline PT_THREAD(pbdrv_block_device_store(struct pt *pt, uint8_t *buffer, uint32_t size, pbio_error_t *err)) {
    PT_BEGIN(pt);
    *err = PBIO_ERROR_NOT_SUPPORTED;
    PT_END(pt);
}
static inline void pbdrv_block_device_set_callback(void (*callback)(void)) {
}

#endif

#endif // _PBDRV_BLOCK_DEVICE_H_

/** @} */

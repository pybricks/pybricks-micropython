// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _PB_FLASH_H_
#define _PB_FLASH_H_

#if (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)

#include <stdint.h>

#include <pbio/error.h>

// Initialize and mount the file system
pbio_error_t pb_flash_init(void);

// Read raw bytes from flash, ignoring file system
pbio_error_t pb_flash_raw_read(uint32_t address, uint8_t *buf, uint32_t size);

// Open one file and get its size.
pbio_error_t pb_flash_file_open_get_size(const char *path, uint32_t *size);

// Read the file that has been opened, all at once, then close.
pbio_error_t pb_flash_file_read(uint8_t *buf, uint32_t size);

// Open a file for writing, write data, and close.
pbio_error_t pb_flash_file_write(const char *path, const uint8_t *buf, uint32_t size);

// Restore a previously backed up firmware file.
pbio_error_t pb_flash_restore_firmware(void);

#endif

#endif // _PB_FLASH_H_

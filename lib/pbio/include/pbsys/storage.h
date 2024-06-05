// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup SysProgramLoad System: Load user programs.
 *
 * Configuration for loading data.
 *
 * @{
 */

#ifndef _PBSYS_STORAGE_H_
#define _PBSYS_STORAGE_H_

#include <stdint.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_STORAGE

// Sanity check that application RAM is enough to load ROM and still do something useful
#if PBSYS_CONFIG_STORAGE_RAM_SIZE < PBSYS_CONFIG_STORAGE_ROM_SIZE + 2048
#error "Application RAM must be at least ROM size + 2K."
#endif

/**
 * Header of loaded data. All data types are little-endian.
 */
typedef struct _pbsys_storage_data_map_t {
    /**
     * How much to write on shutdown (and how much to load on boot). This is
     * reset to 0 in RAM on load, and should be set whenever any data is
     * updated. This must always remain the first element of this structure.
     */
    uint32_t write_size;
    #if PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM
    /**
     * Checksum complement to satisfy bootloader requirements. This ensures
     * that words in the scanned area still add up to precisely 0 after user
     * data was written.
     */
    volatile uint32_t checksum_complement;
    #endif
    /**
     * End-user read-write accessible data.
     */
    uint8_t user_data[PBSYS_CONFIG_STORAGE_USER_DATA_SIZE];
    /**
     * Size of the application program (size of code only).
     */
    uint32_t program_size;
    /**
     * Data of the application program (code + heap).
     */
    uint8_t program_data[] __attribute__((aligned(sizeof(void *))));
} pbsys_storage_data_map_t;

#define PBSYS_STORAGE_MAX_PROGRAM_SIZE (PBSYS_CONFIG_STORAGE_ROM_SIZE - sizeof(pbsys_storage_data_map_t))

pbio_error_t pbsys_storage_set_user_data(uint32_t offset, const uint8_t *data, uint32_t size);

pbio_error_t pbsys_storage_get_user_data(uint32_t offset, uint8_t **data, uint32_t size);

#else

#define PBSYS_STORAGE_MAX_PROGRAM_SIZE (0)

static inline pbio_error_t pbsys_storage_set_user_data(uint32_t offset, const uint8_t *data, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbsys_storage_get_user_data(uint32_t offset, uint8_t **data, uint32_t size) {
    *data = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}


#endif // PBSYS_CONFIG_STORAGE

#endif // _PBSYS_STORAGE_H_

/** @} */

// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2024 The Pybricks Authors

/**
 * @addtogroup SysStorage System: Load user programs, data, and settings.
 *
 * Load user programs, data, and settings.
 *
 * @{
 */

#ifndef _PBSYS_STORAGE_H_
#define _PBSYS_STORAGE_H_

#include <stdint.h>

#include <pbsys/config.h>
#include <pbsys/storage_settings.h>

#if PBSYS_CONFIG_STORAGE

// Sanity check that application RAM is enough to load ROM and still do something useful
#if PBSYS_CONFIG_STORAGE_RAM_SIZE < PBSYS_CONFIG_STORAGE_ROM_SIZE + 2048
#error "Application RAM must be at least ROM size + 2K."
#endif

/**
 * Map of loaded data. All data types are little-endian.
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
     * Firmware version used to create the stored data. See pbio/version.
     * Human-readable when printed as hex. If this value does not match
     * the version of the running firmware, user data will be reset to 0.
     */
    uint32_t stored_firmware_version;
    /**
     * End-user read-write accessible data. Everything after this is also
     * user-readable but not writable.
     */
    uint8_t user_data[PBSYS_CONFIG_STORAGE_USER_DATA_SIZE];
    /**
     * System settings. Settings will be reset to defaults when the firmware
     * version changes due to an update.
     */
    pbsys_storage_settings_t settings;
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

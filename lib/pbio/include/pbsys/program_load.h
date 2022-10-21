// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup SysProgramLoad System: Load user programs.
 *
 * Configuration for loading data.
 *
 * @{
 */

#ifndef _PBSYS_PROGRAM_LOAD_H_
#define _PBSYS_PROGRAM_LOAD_H_

#include <stdint.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_PROGRAM_LOAD

// Sanity check that application RAM is enough to load ROM and still do something useful
#if PBSYS_CONFIG_PROGRAM_LOAD_RAM_SIZE < PBSYS_CONFIG_PROGRAM_LOAD_ROM_SIZE + 2048
#error "Application RAM must be at least ROM size + 2K."
#endif

/**
 * Header of loaded data. All data types are little-endian.
 */
typedef struct _pbsys_program_load_data_header_t {
    /**
     * How much to write on shutdown (and how much to load on boot). This is
     * reset to 0 in RAM on load, and should be set whenever any data is
     * updated. This must always remain the first element of this structure.
     */
    uint32_t write_size;
    #if PBSYS_CONFIG_PROGRAM_LOAD_OVERLAPS_BOOTLOADER_CHECKSUM
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
    uint8_t user_data[PBSYS_CONFIG_PROGRAM_LOAD_USER_DATA_SIZE];
    /**
     * Size of the application program (size of code only).
     */
    uint32_t program_size;
} pbsys_program_load_data_header_t;

#define PBSYS_PROGRAM_LOAD_MAX_PROGRAM_SIZE (PBSYS_CONFIG_PROGRAM_LOAD_ROM_SIZE - sizeof(pbsys_program_load_data_header_t))

#else

#define PBSYS_PROGRAM_LOAD_MAX_PROGRAM_SIZE (0)

#endif // PBSYS_CONFIG_PROGRAM_LOAD

#endif // _PBSYS_PROGRAM_LOAD_H_

/** @} */

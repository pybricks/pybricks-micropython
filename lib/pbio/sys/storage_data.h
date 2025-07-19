// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Data structures for non-volatile storage.

// NB: Normally, pbsys code isn't referenced by drivers, however this is a
// special case where the block device driver needs to have the actual struct
// definition in order to ensure correct memory layout and alignment.

#ifndef _PBSYS_SYS_STORAGE_DATA_H_
#define _PBSYS_SYS_STORAGE_DATA_H_

#include <stdint.h>

#include <pbsys/config.h>
#include <pbsys/storage_settings.h>

/**
 * Information about one code slot.
 *
 * A size of 0 means that this slot is not used. The offset indicates where
 * the program is stored in user storage.
 *
 * Code slots are *not* stored chronologically by slot id. Instead they are
 * stored consecutively as they are received, with the newest program last.
 *
 * If a slot is already in use and a new program should be loaded into it,
 * it is deleted by mem-moving any subsequent programs into its place, and
 * appending the new program to be last again. Since a user is typically only
 * iterating code in one slot, this is therefore usually the last stored
 * program. This means memmoves happen very little, only when needed.
 *
 */
typedef struct {
    uint32_t offset;
    uint32_t size;
} pbsys_storage_slot_info_t;

/**
 * Map of loaded data. All data types are little-endian.
 */
typedef struct {
    /**
     * End-user read-write accessible data. Everything after this is also
     * user-readable but not writable.
     */
    uint8_t user_data[PBSYS_CONFIG_STORAGE_USER_DATA_SIZE];
    /**
     * First 8 symbols of the git hash of the firmware version used to create
     * this data map. If this does not match the version of the running
     * firmware, user data will be reset to 0.
     */
    char stored_firmware_hash[8];
    /**
     * System settings. Settings will be reset to defaults when the firmware
     * version changes due to an update.
     */
    pbsys_storage_settings_t settings;
    /**
     * Size and offset info for each slot.
     */
    pbsys_storage_slot_info_t slot_info[PBSYS_CONFIG_STORAGE_NUM_SLOTS];
    /**
     * Data of the application program (code + heap).
     */
    uint8_t program_data[] __attribute__((aligned(sizeof(void *))));
} pbsys_storage_data_map_t;

#endif // _PBSYS_SYS_STORAGE_DATA_H_

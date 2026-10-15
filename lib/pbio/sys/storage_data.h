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
 * Internal version of the data map below. Bump whenever the layout or the
 * meaning of any field in the map changes, including the platform-specific
 * parts of ::pbsys_storage_settings_t. When the stored version does not match,
 * the data is erased.
 */
#define PBSYS_STORAGE_VERSION ('P' << 24 | 'B' << 16 | 0x0001)

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
     * Version of the layout used to create this data map. If this does not
     * match ::PBSYS_STORAGE_VERSION of the running firmware, everything in
     * this map is reset to 0.
     */
    uint32_t storage_version;
    /**
     * System settings. Settings will be reset to defaults when the storage
     * version changes, not on every firmware update.
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

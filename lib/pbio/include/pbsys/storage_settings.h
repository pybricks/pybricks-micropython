// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

/**
 * @addtogroup SysStorageSettings System: Load user settings.
 *
 * Interface for reading and storing user system settings.
 *
 * @{
 */

#ifndef _PBSYS_STORAGE_SETTINGS_H_
#define _PBSYS_STORAGE_SETTINGS_H_

#include <stdint.h>

#include <pbio/error.h>

#include <pbsys/config.h>

/**
 * System setting flags
 */
typedef enum {
    /**
     * Bluetooth is enabled by the user (defaults to true).
     */
    PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED = (1 << 0),
} pbsys_storage_settings_flags_t;

/**
 * System settings. All data types are little-endian.
 */
typedef struct _pbsys_storage_settings_t {
    uint32_t flags;
} pbsys_storage_settings_t;

#if PBSYS_CONFIG_STORAGE

void pbsys_storage_set_default_settings(pbsys_storage_settings_t *settings);

bool pbsys_storage_settings_bluetooth_enabled(void);

void pbsys_storage_settings_bluetooth_enabled_request_toggle(void);

#else

static inline void pbsys_storage_set_default_settings(pbsys_storage_settings_t *settings) {
}
static inline bool pbsys_storage_settings_bluetooth_enabled(void) {
    return true;
}
static inline void pbsys_storage_settings_bluetooth_enabled_request_toggle(void) {
}

#endif // PBSYS_CONFIG_STORAGE

#endif // _PBSYS_STORAGE_SETTINGS_H_

/** @} */

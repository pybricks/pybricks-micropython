// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_STORAGE

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <pbdrv/bluetooth.h>

#include <pbio/error.h>
#include <pbio/imu.h>
#include <pbsys/status.h>
#include <pbsys/storage.h>
#include <pbsys/storage_settings.h>

#include "storage.h"

/**
 * Sets the default settings after an erase.
 *
 * @param [in]  settings  Settings to populate.
 */
void pbsys_storage_settings_set_defaults(pbsys_storage_settings_t *settings) {
    settings->flags = 0;
    pbsys_storage_settings_set_flag(PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED, true);

    #if PBIO_CONFIG_IMU
    pbio_imu_set_default_settings(&settings->imu_settings);
    #endif // PBIO_CONFIG_IMU

    // Always request save for this one off default setter.
    pbsys_storage_request_write();
}

/**
 * Applies the loaded settings and preferences after boot.
 *
 * @param [in]  settings  Settings to populate.
 */
void pbsys_storage_settings_apply_loaded_settings(pbsys_storage_settings_t *settings) {
    #if PBIO_CONFIG_IMU
    pbio_imu_apply_loaded_settings(&settings->imu_settings);
    #endif // PBIO_CONFIG_IMU
}

/**
 * Gets the value of a flag in the settings.
 *
 * NB: Always returns false if this is called before storage is initalized.
 *
 * @param [in]  flag    The flag to get.
 * @returns             The value of the flag, or false if settings are not available.
 */
bool pbsys_storage_settings_get_flag(pbsys_storage_settings_flags_t flag) {
    pbsys_storage_settings_t *settings = pbsys_storage_settings_get_settings();
    if (!settings) {
        return false;
    }
    return settings->flags & flag;
}

/**
 * Sets the value of a flag in the settings, and requests saving on poweroff if changed.
 *
 * NB: Does nothing if this is called before storage is initalized.
 *
 * @param [in]  flag    The flag to set.
 * @param [in]  value   The value to set.
 */
void pbsys_storage_settings_set_flag(pbsys_storage_settings_flags_t flag, bool value) {
    pbsys_storage_settings_t *settings = pbsys_storage_settings_get_settings();
    if (!settings) {
        return;
    }

    bool current_value = settings->flags & flag;
    if (value == current_value) {
        return;
    }

    if (value) {
        settings->flags |= flag;
    } else {
        settings->flags &= ~flag;
    }
    pbsys_storage_request_write();
}

#endif // PBSYS_CONFIG_STORAGE

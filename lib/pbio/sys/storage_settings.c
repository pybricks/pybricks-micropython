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
    #if PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON
    settings->flags |= PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED;
    #endif
    #if PBIO_CONFIG_IMU
    pbio_imu_set_default_settings(&settings->imu_settings);
    #endif // PBIO_CONFIG_IMU
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

bool pbsys_storage_settings_bluetooth_enabled_get(void) {
    #if PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON
    pbsys_storage_settings_t *settings = pbsys_storage_settings_get_settings();
    if (!settings) {
        return false;
    }
    return settings->flags & PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED;
    #else
    return true;
    #endif // PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON
}
void pbsys_storage_settings_bluetooth_enabled_set(bool enable) {
    #if PBSYS_CONFIG_HMI_PUP_BLUETOOTH_BUTTON
    pbsys_storage_settings_t *settings = pbsys_storage_settings_get_settings();

    bool current_value = pbsys_storage_settings_bluetooth_enabled_get();
    if (enable == current_value) {
        return;
    }

    if (enable) {
        settings->flags |= PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED;
    } else {
        settings->flags &= ~PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED;
    }
    pbsys_storage_request_write();
    #endif
}

#endif // PBSYS_CONFIG_STORAGE

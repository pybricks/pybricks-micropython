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
#include <pbsys/storage_settings.h>

#include "bluetooth.h"
#include "storage.h"

/**
 * Sets the default settings after an erase.
 *
 * @param [in]  settings  Settings to populate.
 */
void pbsys_storage_settings_set_defaults(pbsys_storage_settings_t *settings) {
    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    settings->flags |= PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED;
    #endif
    #if PBIO_CONFIG_IMU
    settings->gyro_stationary_threshold = 5;
    settings->accel_stationary_threshold = 2500;
    settings->heading_correction = 360.0f;
    #endif // PBIO_CONFIG_IMU
}

/**
 * Applies the loaded settings and preferences after boot.
 *
 * @param [in]  settings  Settings to populate.
 */
void pbsys_storage_settings_apply_loaded_settings(pbsys_storage_settings_t *settings) {
    #if PBIO_CONFIG_IMU
    // return value not checked, assumed to be set valid previously
    pbio_imu_set_settings(
        settings->gyro_stationary_threshold,
        settings->accel_stationary_threshold,
        settings->heading_correction
        );
    #endif // PBIO_CONFIG_IMU
}

/**
 * Copies the configured IMU settings to storage and requests them to be saved.
 *
 * @param [in]  settings  Settings to populate.
 */
void pbsys_storage_settings_save_imu_settings(void) {
    pbsys_storage_settings_t *settings = pbsys_storage_settings_get_settings();
    if (!settings) {
        return;
    }
    #if PBIO_CONFIG_IMU
    pbio_imu_get_settings(
        &settings->gyro_stationary_threshold,
        &settings->accel_stationary_threshold,
        &settings->heading_correction
        );
    pbsys_storage_request_write();
    #endif // PBIO_CONFIG_IMU
}

bool pbsys_storage_settings_bluetooth_enabled(void) {
    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    pbsys_storage_settings_t *settings = pbsys_storage_settings_get_settings();
    if (!settings) {
        return true;
    }
    return settings->flags & PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED;
    #else
    return true;
    #endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE
}

void pbsys_storage_settings_bluetooth_enabled_request_toggle(void) {
    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    pbsys_storage_settings_t *settings = pbsys_storage_settings_get_settings();

    // Ignore toggle request in all but idle system status.
    if (!settings
        || pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)
        || pbsys_status_test(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED)
        || pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)
        || pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)
        // Ignore toggle is Bluetooth is currently being used in a connection.
        || pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE)
        || pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)
        // Ignore if last request not yet finished processing.
        || pbsys_storage_settings_bluetooth_enabled() != pbdrv_bluetooth_is_ready()
        ) {
        return;
    }

    // Toggle the user enabled state and poll process to take action.
    settings->flags ^= PBSYS_STORAGE_SETTINGS_FLAGS_BLUETOOTH_ENABLED;
    pbsys_storage_request_write();
    pbsys_bluetooth_process_poll();
    #endif
}

#endif // PBSYS_CONFIG_STORAGE

// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_STORAGE

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <pbdrv/bluetooth.h>

#include <pbio/error.h>
#include <pbsys/status.h>
#include <pbsys/storage_settings.h>

#include "bluetooth.h"
#include "storage.h"

/**
 * Sets the default settings after an erase.
 *
 * @param [in]  settings  Settings to populate.
 */
void pbsys_storage_set_default_settings(pbsys_storage_settings_t *settings) {
    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    settings->bluetooth_ble_user_enabled = true;
    #endif
}

bool pbsys_storage_settings_bluetooth_enabled(void) {
    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    pbsys_storage_settings_t *settings = pbsys_storage_get_settings();
    if (!settings) {
        return true;
    }
    return settings->bluetooth_ble_user_enabled;
    #else
    return true;
    #endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE
}

void pbsys_storage_settings_bluetooth_enabled_request_toggle(void) {
    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    pbsys_storage_settings_t *settings = pbsys_storage_get_settings();

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
        || settings->bluetooth_ble_user_enabled != pbdrv_bluetooth_is_ready()
        ) {
        return;
    }

    // Toggle the user enabled state and poll process to take action.
    settings->bluetooth_ble_user_enabled = !settings->bluetooth_ble_user_enabled;
    pbsys_storage_request_write();
    pbsys_bluetooth_process_poll();
    #endif
}

#endif // PBSYS_CONFIG_STORAGE

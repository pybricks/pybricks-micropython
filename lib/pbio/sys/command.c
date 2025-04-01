// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include <assert.h>
#include <stdint.h>

#include <pbdrv/reset.h>
#include <pbio/protocol.h>
#include <pbsys/command.h>
#include <pbsys/config.h>
#include <pbsys/host.h>
#include <pbsys/storage.h>

#include "./hmi.h"
#include "./storage.h"
#include "./program_stop.h"

static pbsys_command_write_app_data_callback_t write_app_data_callback = NULL;

/**
 * Sets callback for the write program data buffer command.
 *
 * @param [in]  callback  The callback to set or @c NULL to unset.
 */
void pbsys_command_set_write_app_data_callback(pbsys_command_write_app_data_callback_t callback) {
    write_app_data_callback = callback;
}

/**
 * Parses binary data for command and dispatches handler for command.
 * @param [in]  data    The raw command data.
 * @param [in]  size    The size of @p data in bytes.
 */
pbio_pybricks_error_t pbsys_command(const uint8_t *data, uint32_t size) {
    assert(data);
    assert(size);

    pbio_pybricks_command_t cmd = data[0];

    switch (cmd) {

        case PBIO_PYBRICKS_COMMAND_STOP_USER_PROGRAM:
            pbsys_program_stop(false);
            return PBIO_PYBRICKS_ERROR_OK;

        case PBIO_PYBRICKS_COMMAND_START_USER_PROGRAM: {
            if (size > 2) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }
            // Use payload as program ID, otherwise use active user slot.
            return pbio_pybricks_error_from_pbio_error(
                pbsys_main_program_request_start((size == 2 ? data[1] : pbsys_hmi_get_selected_program_slot()), PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_REMOTE));
        }
        #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL
        case PBIO_PYBRICKS_COMMAND_START_REPL:
            // Deprecated. For backwards compatibility with Pybricks
            // Profile < v1.4.0, make it work anyway.
            return pbio_pybricks_error_from_pbio_error(
                pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_REPL, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_REMOTE));
        #endif // PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL

        case PBIO_PYBRICKS_COMMAND_WRITE_USER_PROGRAM_META:
            return pbio_pybricks_error_from_pbio_error(pbsys_storage_set_program_size(
                pbio_get_uint32_le(&data[1])));

        case PBIO_PYBRICKS_COMMAND_WRITE_USER_RAM:
            return pbio_pybricks_error_from_pbio_error(pbsys_storage_set_program_data(
                pbio_get_uint32_le(&data[1]), &data[5], size - 5));

        case PBIO_PYBRICKS_COMMAND_REBOOT_TO_UPDATE_MODE:
            pbdrv_reset(PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE);
            return PBIO_PYBRICKS_ERROR_OK;

        case PBIO_PYBRICKS_COMMAND_WRITE_STDIN:
            #if PBSYS_CONFIG_HOST
            if (pbsys_host_rx_get_free() < size - 1) {
                return PBIO_PYBRICKS_ERROR_BUSY;
            }
            pbsys_host_rx_write(&data[1], size - 1);
            #endif
            // If no consumers are configured, goes to "/dev/null" without error
            return PBIO_PYBRICKS_ERROR_OK;

        case PBIO_PYBRICKS_COMMAND_WRITE_APP_DATA: {
            if (!write_app_data_callback) {
                // No errors when no consumer is configured. This avoids errors
                // when data is sent after the program ends.
                return PBIO_PYBRICKS_ERROR_OK;
            }

            // Requires at least the message type and data offset.
            if (size <= 3) {
                return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
            }

            uint16_t offset = pbio_get_uint16_le(&data[1]);
            uint16_t data_size = size - 3;
            const uint8_t *data_to_write = &data[3];
            return pbio_pybricks_error_from_pbio_error(write_app_data_callback(offset, data_size, data_to_write));
        }
        default:
            return PBIO_PYBRICKS_ERROR_INVALID_COMMAND;
    }
}

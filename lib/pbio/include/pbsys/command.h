// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2024 The Pybricks Authors

/**
 * @addtogroup SysCommand System: Command Parser
 *
 * This module provides a command parser to handle commands from various sources
 * (e.g. Bluetooth, USB, internal, etc.).
 *
 * @{
 */

#ifndef _PBSYS_COMMAND_H_
#define _PBSYS_COMMAND_H_

#include <stdint.h>

#include <pbio/protocol.h>
#include <pbsys/main.h>

typedef enum {
    // NB: these values allow passing pbsys_command_transport_t directly as
    // pbsys_main_program_start_request_type_t without a lookup table.
    PBSYS_COMMAND_TRANSPORT_BLE = PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BLUETOOTH,
    PBSYS_COMMAND_TRANSPORT_USB = PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_USB,
} pbsys_command_transport_t;

pbio_pybricks_error_t pbsys_command(const uint8_t *data, uint32_t size, pbsys_command_transport_t transport);

/**
 * Callback called when the write app data command is given by the host.
 *
 * @param [in]  offset  Offset from the start of the buffer.
 * @param [in]  size    Size of the data to be written.
 * @param [in]  data    The data to write.
 * @returns             Error code.
 */
typedef pbio_error_t (*pbsys_command_write_app_data_callback_t)(uint16_t offset, uint32_t size, const uint8_t *data);

void pbsys_command_set_write_app_data_callback(pbsys_command_write_app_data_callback_t callback);

#endif // _PBSYS_COMMAND_H_

/** @} */

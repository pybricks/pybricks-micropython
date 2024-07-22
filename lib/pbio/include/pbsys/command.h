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

pbio_pybricks_error_t pbsys_command(const uint8_t *data, uint32_t size);

/**
 * Callback called when the write app data command is given by the host.
 *
 * @param [in]  offset  Offset from the start of the buffer.
 * @param [in]  size    Size of the data to be written.
 * @param [in]  data    The data to write.
 */
typedef void (*pbsys_command_write_app_data_callback_t)(uint16_t offset, uint32_t size, const uint8_t *data);

void pbsys_command_set_write_app_data_callback(pbsys_command_write_app_data_callback_t callback);

#endif // _PBSYS_COMMAND_H_

/** @} */

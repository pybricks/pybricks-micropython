// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

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

void pbsys_command(const uint8_t *data, uint32_t size);

#endif // _PBSYS_COMMAND_H_

/** @} */

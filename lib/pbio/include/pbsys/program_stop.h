// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup SysProgramStop System: Stop user programs.
 *
 * This module configures how user programs can be stopped.
 *
 * @{
 */

#ifndef _PBSYS_PROGRAM_STOP_H_
#define _PBSYS_PROGRAM_STOP_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/config.h>
#include <pbio/button.h>
#include <pbio/error.h>

void pbsys_program_stop_set_buttons(pbio_button_flags_t buttons);

#endif // _PBSYS_PROGRAM_STOP_H_

/** @} */

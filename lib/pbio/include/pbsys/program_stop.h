// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

/**
 * @addtogroup SysProgramStop System: Stop user programs.
 *
 * This module configures how user programs can be stopped.
 *
 * @{
 */

#ifndef _PBSYS_PROGRAM_STOP_H_
#define _PBSYS_PROGRAM_STOP_H_

#include <pbio/button.h>
#include <pbsys/config.h>

#if PBSYS_CONFIG_PROGRAM_STOP

void pbsys_program_stop(bool force_stop);
pbio_button_flags_t pbsys_program_stop_get_buttons(void);
void pbsys_program_stop_set_buttons(pbio_button_flags_t buttons);

#else // PBSYS_CONFIG_PROGRAM_STOP

static inline void pbsys_program_stop_set_buttons(pbio_button_flags_t buttons) {
}

#endif // PBSYS_CONFIG_PROGRAM_STOP

#endif // _PBSYS_PROGRAM_STOP_H_

/** @} */

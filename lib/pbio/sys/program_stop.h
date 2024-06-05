// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2024 The Pybricks Authors

#ifndef _PBSYS_SYS_PROGRAM_STOP_H_
#define _PBSYS_SYS_PROGRAM_STOP_H_

#include <pbsys/config.h>

#if PBSYS_CONFIG_PROGRAM_STOP

void pbsys_program_stop_poll(void);
void pbsys_program_stop(bool force_stop);

#else // PBSYS_CONFIG_PROGRAM_STOP

static inline void pbsys_program_stop_poll(void) {
}

static inline void pbsys_program_stop(bool force_stop) {
}

#endif // PBSYS_CONFIG_PROGRAM_STOP

#endif // _PBSYS_SYS_PROGRAM_STOP_H_

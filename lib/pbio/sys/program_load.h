// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _PBSYS_SYS_PROGRAM_LOAD_H_
#define _PBSYS_SYS_PROGRAM_LOAD_H_

#include <pbsys/config.h>
#include <pbsys/program_load.h>

#if PBSYS_CONFIG_PROGRAM_LOAD

void pbsys_program_load_receive_start(void);
bool pbsys_program_load_receive_complete(void);
pbsys_program_load_info_t *pbsys_program_load_get_info(void);

#else

static inline void pbsys_program_load_receive_start(void) {
}
static inline bool pbsys_program_load_receive_complete(void) {
    return false;
}
static inline pbsys_program_load_info_t *pbsys_program_load_get_info(void) {
    return NULL;
}

#endif // PBSYS_CONFIG_PROGRAM_LOAD

#endif // _PBSYS_SYS_PROGRAM_LOAD_H_

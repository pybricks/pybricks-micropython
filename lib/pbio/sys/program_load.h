// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _PBSYS_SYS_PROGRAM_LOAD_H_
#define _PBSYS_SYS_PROGRAM_LOAD_H_

#include <pbio/error.h>

#include <pbsys/config.h>
#include <pbsys/main.h>

#include <stdint.h>

#if PBSYS_CONFIG_PROGRAM_LOAD

void pbsys_program_load_init(void);
void pbsys_program_load_deinit(void);
pbio_error_t pbsys_program_load_receive(pbsys_main_program_t *program);

#else
static inline void pbsys_program_load_init(void) {
}
static inline void pbsys_program_load_deinit(void) {
}
static inline pbio_error_t pbsys_program_load_receive(pbsys_main_program_t *program) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBSYS_CONFIG_PROGRAM_LOAD

#endif // _PBSYS_SYS_PROGRAM_LOAD_H_

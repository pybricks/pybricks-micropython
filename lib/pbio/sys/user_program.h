// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2024 The Pybricks Authors

#ifndef _PBSYS_SYS_USER_PROGRAM_H_
#define _PBSYS_SYS_USER_PROGRAM_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbsys/config.h>
#include <pbsys/main.h>

#if PBSYS_CONFIG_USER_PROGRAM

pbio_error_t pbsys_user_program_wait_command(pbsys_main_program_t *program);
pbio_error_t pbsys_user_program_start_program(void);
pbio_error_t pbsys_user_program_start_repl(void);

#else

static inline pbio_error_t pbsys_user_program_wait_command(pbsys_main_program_t *program) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_user_program_start_program(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_user_program_start_repl(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBSYS_CONFIG_USER_PROGRAM

#endif // _PBSYS_SYS_USER_PROGRAM_H_

// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _PBSYS_SYS_PROGRAM_LOAD_H_
#define _PBSYS_SYS_PROGRAM_LOAD_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbsys/config.h>
#include <pbsys/main.h>

#if PBSYS_CONFIG_PROGRAM_LOAD

void pbsys_program_load_init(void);
void pbsys_program_load_deinit(void);
pbio_error_t pbsys_program_load_receive(pbsys_main_program_t *program);
pbio_error_t pbsys_program_load_set_program_size(uint32_t size);
pbio_error_t pbsys_program_load_set_program_data(uint32_t offset, const void *data, uint32_t size);
pbio_error_t pbsys_program_load_start_user_program(void);
pbio_error_t pbsys_program_load_start_repl(void);

#else
static inline void pbsys_program_load_init(void) {
}
static inline void pbsys_program_load_deinit(void) {
}
static inline pbio_error_t pbsys_program_load_receive(pbsys_main_program_t *program) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_program_load_set_program_size(uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_program_load_set_program_data(uint32_t offset, const void *data, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_program_load_start_user_program(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_program_load_start_repl(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBSYS_CONFIG_PROGRAM_LOAD

#endif // _PBSYS_SYS_PROGRAM_LOAD_H_

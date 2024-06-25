// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _PBSYS_SYS_STORAGE_H_
#define _PBSYS_SYS_STORAGE_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbsys/config.h>
#include <pbsys/main.h>
#include <pbsys/storage_settings.h>

#if PBSYS_CONFIG_STORAGE

void pbsys_storage_init(void);
void pbsys_storage_deinit(void);
pbio_error_t pbsys_storage_set_program_size(uint32_t size);
pbio_error_t pbsys_storage_set_program_data(uint32_t offset, const void *data, uint32_t size);
pbio_error_t pbsys_storage_assert_program_valid(void);
void pbsys_storage_get_program_data(pbsys_main_program_t *program);
pbsys_storage_settings_t *pbsys_storage_settings_get_settings(void);
void pbsys_storage_request_write(void);

#else
static inline void pbsys_storage_init(void) {
}
static inline void pbsys_storage_deinit(void) {
}
static inline pbsys_storage_settings_t *pbsys_storage_settings_get_settings(void) {
    return NULL;
}
static inline pbio_error_t pbsys_storage_set_program_size(uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_storage_set_program_data(uint32_t offset, const void *data, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_storage_assert_program_valid(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbsys_storage_get_program_data(pbsys_main_program_t *program) {
}
static inline void pbsys_storage_request_write(void) {
}

#endif // PBSYS_CONFIG_STORAGE

#endif // _PBSYS_SYS_STORAGE_H_

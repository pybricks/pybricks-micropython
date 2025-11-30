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
bool pbsys_storage_slot_change_is_allowed(void);
void pbsys_storage_get_program_data(pbsys_main_program_t *program);
pbsys_storage_settings_t *pbsys_storage_settings_get_settings(void);

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
static inline bool pbsys_storage_slot_change_is_allowed(void) {
    return false;
}
static inline void pbsys_storage_get_program_data(pbsys_main_program_t *program) {
    program->code_start = NULL;
    program->code_end = NULL;

    // Must be defined in linker script.
    extern uint8_t pbsys_storage_heap_start;
    extern uint8_t pbsys_storage_heap_end;

    program->user_ram_start = &pbsys_storage_heap_start;
    program->user_ram_end = &pbsys_storage_heap_end;
}

#endif // PBSYS_CONFIG_STORAGE

#endif // _PBSYS_SYS_STORAGE_H_

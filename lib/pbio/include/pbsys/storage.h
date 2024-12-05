// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2024 The Pybricks Authors

/**
 * @addtogroup SysStorage System: Load user programs, data, and settings.
 *
 * Load user programs, data, and settings.
 *
 * @{
 */

#ifndef _PBSYS_STORAGE_H_
#define _PBSYS_STORAGE_H_

#include <stdint.h>

#include <pbsys/config.h>
#include <pbsys/storage_settings.h>

#if PBSYS_CONFIG_STORAGE

uint32_t pbsys_storage_get_maximum_program_size(void);

void pbsys_storage_reset_storage(void);

pbio_error_t pbsys_storage_set_user_data(uint32_t offset, const uint8_t *data, uint32_t size);

pbio_error_t pbsys_storage_get_user_data(uint32_t offset, uint8_t **data, uint32_t size);

void pbsys_storage_request_write(void);

#else

static inline uint32_t pbsys_storage_get_maximum_program_size(void) {
    return 0;
}

static inline void pbsys_storage_reset_storage(void) {
}

static inline pbio_error_t pbsys_storage_set_user_data(uint32_t offset, const uint8_t *data, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_storage_get_user_data(uint32_t offset, uint8_t **data, uint32_t size) {
    *data = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbsys_storage_request_write(void) {
}

#endif // PBSYS_CONFIG_STORAGE

#endif // _PBSYS_STORAGE_H_

/** @} */

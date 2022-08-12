// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup SystemMain System: Main Program
 * @{
 */

#ifndef _PBSYS_MAIN_H_
#define _PBSYS_MAIN_H_

#include <pbsys/config.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * Main application program data information.
 */
typedef struct _pbsys_main_program_t {
    /**
     * User program data.
     */
    uint8_t *data;
    /**
     * User program data size.
     */
    uint32_t size;
    /**
     * Whether to run an application-specific builtin program instead of the
     * program given by the data. The builtin program may still use the data.
     */
    bool run_builtin;
} pbsys_main_program_t;

#if PBSYS_CONFIG_MAIN

void pbsys_main_application(pbsys_main_program_t *program);

#endif // PBSYS_CONFIG_MAIN

#endif // _PBSYS_MAIN_H_

/** @} */

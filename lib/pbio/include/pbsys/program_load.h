// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

/**
 * @addtogroup SysUserProgram System: User Program Hooks
 *
 * This module provides hooks for starting and stopping user programs on top
 * of the pbsys runtime.
 *
 * @{
 */

#ifndef _PBSYS_PROGRAM_LOAD_H_
#define _PBSYS_PROGRAM_LOAD_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/config.h>

/**
 * Active user program type.
 */
typedef enum {
    /**
     * Run the currently stored program.
     */
    PSYS_PROGRAM_LOAD_TYPE_STORED = 0,
    /**
     * Indicator to run appplication-specific special program.
     */
    PSYS_PROGRAM_LOAD_TYPE_BUILTIN_0 = 0x20202020,
    /**
     * There is no valid program to run.
     */
    PSYS_PROGRAM_LOAD_TYPE_NONE = UINT32_MAX,
} pbsys_program_load_type_t;

/**
 * Struct to hold system resource information for use by application program.
 */
typedef struct {
    /**
     * Type of program to run next.
     */
    pbsys_program_load_type_t program_type;
    /**
     * Start of the heap available to the application.
     */
    uint8_t *appl_heap_start;
    /**
     * End of the heap available to the system and application.
     */
    uint8_t *const sys_heap_end;
    /**
     * Start of the system stack.
     */
    uint8_t *const sys_stack_start;
    /**
     * End of the system stack.
     */
    uint8_t *const sys_stack_end;
    /**
     * Application-specific program data. Used only when type
     * is ::PSYS_PROGRAM_LOAD_TYPE_STORED.
     */
    uint8_t *program_data;
    /**
     * Application data size. Used only when type
     * is ::PSYS_PROGRAM_LOAD_TYPE_STORED.
     */
    uint32_t program_size;
} pbsys_program_load_info_t;

void pbsys_program_load_application_main(pbsys_program_load_info_t *info);

#endif // _PBSYS_PROGRAM_LOAD_H_

/** @} */

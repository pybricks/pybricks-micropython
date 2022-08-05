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
     * There is no valid program to run.
     */
    PSYS_PROGRAM_LOAD_TYPE_NONE = 0,
    /**
     * This is is a valid user program.
     */
    PSYS_PROGRAM_LOAD_TYPE_NORMAL = 1,
    /**
     * Indicator to run appplication-specific special program.
     */
    PSYS_PROGRAM_LOAD_TYPE_BUILTIN_0 = 0x20202020,
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
     * Start of the heap available to the program.
     */
    uint8_t *heap_start;
    /**
     * End of the heap available to the program.
     */
    uint8_t *heap_end;
    /**
     * Start of the system stack.
     */
    uint8_t *stack_start;
    /**
     * End of the system stack.
     */
    uint8_t *stack_end;
    /**
     * Application-specific program data. Used only when type
     * is ::PSYS_PROGRAM_LOAD_TYPE_NORMAL.
     */
    uint8_t *program_data;
    /**
     * Application data size. Used only when type
     * is ::PSYS_PROGRAM_LOAD_TYPE_NORMAL.
     */
    uint32_t program_size;
    /**
     * Maximum application data size.
     */
    uint32_t program_size_max;
} pbsys_program_load_info_t;

void pbsys_program_load_application_main(pbsys_program_load_info_t *info);

#endif // _PBSYS_PROGRAM_LOAD_H_

/** @} */

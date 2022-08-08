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

#ifndef _PBSYS_USER_PROGRAM_H_
#define _PBSYS_USER_PROGRAM_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/config.h>
#include <pbio/button.h>
#include <pbio/error.h>

/**
 * Callback function to handle stop button press during user program.
 */
typedef void (*pbsys_user_program_stop_t)(void);

/**
 * Callback function to handle stdin events.
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be placed
 *                  in the stdin buffer, otherwise *false*.
 */
typedef bool (*pbsys_user_program_stdin_event_callback_t)(uint8_t c);

/**
 * Struct to hold callback functions for user programs.
 */
typedef struct {
    /**
     * Optional function that will be called if the stop button is pressed
     * before ::pbsys_user_program_unprepare() is called.
     */
    pbsys_user_program_stop_t stop;

    /**
     * Optional function that will be called when stdin has new data to be read.
     *
     * This can be used, for example, to implement a keyboard interrupt when
     * CTRL+C is pressed.
     */
    pbsys_user_program_stdin_event_callback_t stdin_event;
} pbsys_user_program_callbacks_t;

void pbsys_user_program_prepare(const pbsys_user_program_callbacks_t *callbacks);
void pbsys_user_program_unprepare(void);
void pbsys_user_program_stop(void);
void pbsys_user_program_set_stop_buttons(pbio_button_flags_t buttons);

/**
 * Active user program type.
 */
typedef enum {
    /**
     * There is no valid program to run.
     */
    PSYS_USER_PROGRAM_TYPE_NONE = 0,
    /**
     * This is is a valid user program.
     */
    PSYS_USER_PROGRAM_TYPE_NORMAL = 1,
    /**
     * Indicator to run appplication-specific special program.
     */
    PSYS_USER_PROGRAM_TYPE_BUILTIN_0 = 0x20202020,
} pbsys_user_program_type_t;

/**
 * Struct to hold system resource information for use by application program.
 */
typedef struct {
    /**
     * Type of program to run next.
     */
    pbsys_user_program_type_t program_type;
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
     * is ::PSYS_USER_PROGRAM_TYPE_NORMAL.
     */
    uint8_t *program_data;
    /**
     * Application data size. Used only when type
     * is ::PSYS_USER_PROGRAM_TYPE_NORMAL.
     */
    uint32_t program_size;
    /**
     * Maximum application data size.
     */
    uint32_t program_size_max;
} pbsys_user_program_info_t;


#endif // _PBSYS_USER_PROGRAM_H_

/** @} */

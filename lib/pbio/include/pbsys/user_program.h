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

#endif // _PBSYS_USER_PROGRAM_H_

/** @} */

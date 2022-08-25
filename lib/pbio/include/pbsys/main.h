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

/**
 * Callback function to handle stdin events.
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be placed
 *                  in the stdin buffer, otherwise *false*.
 */
typedef bool (*pbsys_main_stdin_event_callback_t)(uint8_t c);

#if PBSYS_CONFIG_MAIN

/**
 * Runs the main application program.
 *
 * This should be provided by the application running on top of pbio.
 *
 * @param [in]  program   Program size and data
 */
void pbsys_main_run_program(pbsys_main_program_t *program);

/**
 * Stops (cancels) the main application program.
 *
 * This should be provided by the application running on top of pbio.
 */
void pbsys_main_stop_program(void);

/**
 * Handles standard input.
 *
 * This should be provided by the application running on top of pbio.
 *
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be
 *                  placed in the stdin buffer, otherwise *false*.
 */
bool pbsys_main_stdin_event(uint8_t c);

#else

static inline void pbsys_main_stop_program(void) {
}

static inline bool pbsys_main_stdin_event(uint8_t c) {
    return false;
}

#endif // PBSYS_CONFIG_MAIN

#endif // _PBSYS_MAIN_H_

/** @} */

// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

/**
 * \addtogroup Sys System-level Functions
 *
 * This module provides platform-specific system-level functions for platforms
 * where the Pybricks I/O library also acts as the operating system for a
 * programmable brick.
 *
 * These functions have no effect on systems with an independant operating
 * system, e.g. ev3dev on LEGO MINDSTORMS EV3.
 * @{
 */

#ifndef _PBSYS_SYS_H_
#define _PBSYS_SYS_H_

#include <stdbool.h>
#include <stdint.h>

#include "pbio/error.h"
#include "sys/process.h"

/**
 * Callback function to handle stop button press during user program.
 */
typedef void (*pbsys_stop_callback_t)(void);

/**
 * Callback function to handle stdin events.
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be placed
 *                  in the stdin buffer, otherwise *false*.
 */
typedef bool (*pbsys_stdin_event_callback_t)(uint8_t c);

/**
 * Struct to hold callback functions for user programs.
 */
typedef struct {
    /**
     * Optional function that will be called if the stop button is pressed
     * before ::pbsys_unprepare_user_program() is called.
     */
    pbsys_stop_callback_t stop;

    /**
     * Optional function that will be called when stdin has new data to be read.
     *
     * This can be used, for example, to implement a keyboard interrupt when
     * CTRL+C is pressed.
     */
    pbsys_stdin_event_callback_t stdin_event;
} pbsys_user_program_callbacks_t;

#ifdef PBIO_CONFIG_ENABLE_SYS

/**
 * Performs platform-specific preperation for running a user program.
 * @param [in]  callbacks   Optional struct of callback function pointers.
 */
void pbsys_prepare_user_program(const pbsys_user_program_callbacks_t *callbacks);

/**
 * Performs platform-specific cleanup/reset after running a user program.
 */
void pbsys_unprepare_user_program(void);

/**
 * Read one character from stdin.
 * @param [out] c       The character read
 * @return              ::PBIO_SUCCESS if a character was available,
 *                      ::PBIO_ERROR_AGAIN if no character was available to be
 *                      read at this time or ::PBIO_ERROR_NOT_SUPPORTED if the
 *                      platform does not have a stdin.
 */
pbio_error_t pbsys_stdin_get_char(uint8_t *c);

/**
 * Write one character to stdout.
 * @param [in] c        The character to write
 * @return              ::PBIO_SUCCESS if a character was written,
 *                      ::PBIO_ERROR_AGAIN if the character could not be written
 *                      at this time or ::PBIO_ERROR_NOT_SUPPORTED if the
 *                      platform does not have a stdout.
 */
pbio_error_t pbsys_stdout_put_char(uint8_t c);

/**
 * Reboots the brick. This could also be considered a "hard" reset. This
 * function never returns.
 * @param fw_update     If *true*, system will reboot into firmware update mode,
 *                      otherise it will reboot normally.
 */
void pbsys_reboot(bool fw_update) __attribute__((noreturn));

/**
 * Powers off the brick. This function never returns.
 */
void pbsys_power_off(void) __attribute__((noreturn));

/** @cond INTERNAL */

PROCESS_NAME(pbsys_process);

/** @endcond */

#else

static inline void pbsys_prepare_user_program(const pbsys_user_program_callbacks_t *callbacks) { }
static inline void pbsys_unprepare_user_program(void) { }
static inline pbio_error_t pbsys_stdin_get_char(uint8_t *c) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbsys_stdout_put_char(uint8_t c) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline void pbsys_reset(void) { }
static inline void pbsys_reboot(bool fw_update) { }
static inline void pbsys_power_off(void) { }

#endif

#endif // _PBSYS_SYS_H_

/** @}*/

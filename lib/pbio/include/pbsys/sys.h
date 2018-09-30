
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

/**
 * Callback function to handle stop button press during user program.
 */
typedef void (*pbsys_stop_callback_t)(void);

#ifdef PBIO_CONFIG_ENABLE_SYS

/**
 * Performs platform-specific preperation for running a user program.
 * @param [in]  stop_func   Optional function that will be called if the stop
 *                          button is pressed before ::pbsys_unprepare_user_program()
 *                          is called.
 */
void pbsys_prepare_user_program(pbsys_stop_callback_t stop_func);

/**
 * Performs platform-specific cleanup/reset after running a user program.
 */
void pbsys_unprepare_user_program(void);

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

/**
 * Initialize system.
 */
void _pbsys_init(void);

/**
 * Checks for pending system events, such as low battery.
 */
void _pbsys_poll(uint32_t now);

/** @endcond */

#else

static void pbsys_prepare_user_program(pbsys_stop_callback_t stop_func) { }
static void pbsys_unprepare_user_program(void) { }
static inline void pbsys_reset(void) { }
static inline void pbsys_reboot(bool fw_update) { }
static inline void pbsys_power_off(void) { }
static inline void _pbsys_init(void) { }
static inline void _pbsys_poll(uint32_t now) { }

#endif

#endif // _PBSYS_SYS_H_

/** @}*/

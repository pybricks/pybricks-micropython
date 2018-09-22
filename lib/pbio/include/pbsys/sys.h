
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

#ifdef PBIO_CONFIG_ENABLE_SYS

/**
 * Performs platform-specific preperation for running a user program.
 */
void pbsys_prepare_user_program(void);

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
void pbsys_reboot(bool fw_update);

/**
 * Powers off the brick. This function never returns.
 */
void pbsys_power_off(void);

/** @cond INTERNAL */

/**
 * Checks for pending system events, such as low battery.
 */
void _pbsys_poll(uint32_t now);

/** @endcond */

#else

static inline void pbsys_reset(void) { }
static inline void pbsys_reboot(bool) { }
static inline void pbsys_power_off(void) { }
static inline void _pbsys_poll(void) { }

#endif

#endif // _PBSYS_SYS_H_

/** @}*/

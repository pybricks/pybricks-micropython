
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
     * WARNING: This can be called in an interrupt context.
     *
     * This can be used, for example, to implement a keyboard interrupt when CTRL+C
     * is pressed.
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

/**
 * Calls the user program *stdin_event* function.
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be placed
 *                  in the stdin buffer, otherwise *false*.
 */
bool _pbsys_stdin_irq(uint8_t c);

/** @endcond */

#else

static inline void pbsys_reset(void) { }
static inline void pbsys_reboot(bool fw_update) { }
static inline void pbsys_power_off(void) { }
static inline void _pbsys_init(void) { }
static inline void _pbsys_poll(uint32_t now) { }
static inline bool _pbsys_stdin_irq(uint8_t c) { return false; }

#endif

#endif // _PBSYS_SYS_H_

/** @}*/

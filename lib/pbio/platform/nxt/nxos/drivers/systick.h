/** @file systick.h
 *  @brief System timer interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_SYSTICK_H__
#define __NXOS_BASE_DRIVERS_SYSTICK_H__

#include <stdint.h>

/** @addtogroup driver */
/*@{*/

/** @defgroup systick System timer
 *
 * The system timer is in charge of (surprise!) keeping system time. It
 * counts the number of milliseconds elapsed since bootup, provides a
 * few busy waiting functions, and allows application kernels to install
 * a scheduling callback that will run periodically.
 */
/*@{*/

/** Sleep for @a ms milliseconds.
 *
 * @param ms The number of milliseconds to sleep.
 *
 * @note As the Baseplate provides no scheduler, this sleeping is a busy
 * wait loop.
 */
void nx_systick_wait_ms(uint32_t ms);

/** Sleep for approximately @a ns nanoseconds.
 *
 * @param ns The number of nanoseconds to sleep.
 *
 * @note This sleep routine is a busy loop whose accuracy is based
 * entirely on the instruction timings and pipeline delays in the ARM7
 * cpu. It may not be exact.
 */
void nx_systick_wait_ns(uint32_t ns);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_SYSTICK_H__ */

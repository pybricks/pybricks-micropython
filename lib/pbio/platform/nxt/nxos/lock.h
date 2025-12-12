/** @file lock.h
 *  @brief Locking primitives.
 */

/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_LOCK_H__
#define __NXOS_BASE_LOCK_H__

#include <stdint.h>

/** @addtogroup kernel */
/*@{*/

/** @defgroup locking Locking primitives
 *
 * Since the base kernel does not provide a scheduler, complex
 * synchronization primitives cannot be provided. On the other hand, the
 * kernel does provide some basic functions that can be of use to
 * implement higher level primitives.
 */
/*@{*/

/** @name Atomic memory access
 *
 * These functions provide thin wrappers around the ARM7 atomic swapping
 * operations. These are guaranteed by the architecture to be atomic,
 * since the memory bus is kept locked for a read plus a write.
 **/
/*@{*/

/** Atomically write @a val at @a dest, and return the previous value.
 *
 * @param dest The address of the value to write.
 * @param val The new value to write.
 * @return The previous value at @a dest.
 */
uint32_t nx_atomic_cas32(uint32_t *dest, uint32_t val);

/** Atomically write @a val at @a dest, and return the previous value.
 *
 * Same as ns_atomic_cas32(), but for an 8-bit value.
 *
 * @param dest The address of the value to write.
 * @param val The new value to write.
 * @return The previous value at @a dest.
 *
 * @sa nx_atomic_cas32
 */
uint8_t nx_atomic_cas8(uint8_t *dest, uint8_t val);

/*@}*/
/*@}*/

#endif /* __NXOS_LOCK_H__ */

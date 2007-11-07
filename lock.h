/** @file lock.h
 *  @brief Locking primitives
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_LOCK_H__
#define __NXOS_BASE_LOCK_H__

#include "base/types.h"

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
U32 nx_atomic_cas32(U32 *dest, U32 val);

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
U8 nx_atomic_cas8(U8 *dest, U8 val);

/*@}*/

/** @name Spinlocks
 *
 * A spinlock is a mutex that can be locked and unlocked. Unlike a
 * mutex, attempting to lock an already locked mutex doesn't cause
 * preemption (since there is no scheduler), but loops forever until the
 * spinlock can be acquired.
 *
 * @note This means that a spinlock is only useful when you need to
 * synchronize between the main execution context and an interrupt
 * handler, or some other form of preemption that can break free of the
 * spinlock's infinite loop.
 */
/*@{*/

/** The basic spinlock type. */
typedef volatile U8 spinlock;

/** Initial value for an unlocked spinlock. */
#define SPINLOCK_INIT_UNLOCKED 0

/** Initial value for a locked spinlock. */
#define SPINLOCK_INIT_LOCKED 1

/** @cond DOXYGEN_SKIP */
void nx_spinlock_acquire_from_ref(spinlock *lock);
bool nx_spinlock_try_acquire_from_ref(spinlock *lock);
/** @endcond */

/** Acquire @a lock and return.
 *
 * Will loop indefinitely until the spinlock can be acquired.
 *
 * @param lock The spinlock to acquire.
 */
#define nx_spinlock_acquire(lock) spinlock_acquire_from_ref(&(lock))

/** Attempt to acquire @a lock and return the result.
 *
 * Unlike nx_spinlock_acquire(), this function does not block, but may
 * fail to acquire the spinlock.
 *
 * @param lock The spinlock to acquire.
 * @return 1 if the spinlock was acquired, 0 if it was already locked.
 */
#define nx_spinlock_try_acquire(lock) spinlock_try_acquire_from_ref(&(lock))

/** Release @a lock.
 *
 * Does not block.
 *
 * @param lock The spinlock to release.
 */
#define nx_spinlock_release(lock) { lock = 0; }

/*@}*/
/*@}*/

#endif /* __NXOS_LOCK_H__ */

#ifndef __NXOS_LOCK_H__
#define __NXOS_LOCK_H__

#include "base/types.h"

typedef volatile U8 spinlock; /* Basic spinlock type. */

#define SPINLOCK_INIT_UNLOCKED 0
#define SPINLOCK_INIT_LOCKED 1

extern U32 nx_atomic_cas32(U32 *dest, U32 val);
extern U8 nx_atomic_cas8(U8 *dest, U8 val);

extern void nx_spinlock_acquire_from_ref(spinlock *lock);
#define nx_spinlock_acquire(lock) spinlock_acquire_from_ref(&(lock))
extern bool nx_spinlock_try_acquire_from_ref(spinlock *lock);
#define nx_spinlock_try_acquire(lock) spinlock_try_acquire_from_ref(&(lock))
#define nx_spinlock_release(lock) { lock = 0; }

#endif /* __NXOS_LOCK_H__ */

/** @file memalloc.h
 *  @brief Memory allocator.
 */

/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_MEMALLOC_H__
#define __NXOS_BASE_MEMALLOC_H__

#include "base/types.h"

/** @addtogroup lib */
/*@{*/

/** @defgroup memalloc Memory allocator
 *
 * This optional library provides the TLSF memory allocator to
 * application kernels. The TLSF allocator performs all allocation
 * functions in constant (O(1)) time and is optimized to minimize
 * memory fragmentation, making it ideal for low resource embedded
 * systems such as the NXT.
 *
 * @note If you do want to use the allocator, you will first need to
 * initialize it by calling nx_mem_init() or nx_mem_init_full(). All the
 * other functions of the allocator assume that the allocator is
 * initialized.
 *
 * @warning The memory allocator is @b not safe for concurrent
 * access. You must provide your own locking around it if you are
 * going to use it from concurrent contexts. Also be aware that this
 * means you cannot use the allocator from within interrupt handlers
 * (accessing already allocated memory is fine).
 */
/*@{*/

/** @name Controlling the allocator */
/*@{*/

/** Initialize the allocator's memory pool.
 *
 * After this call, the memory defined in memmap.h as userspace (@a
 * NX_USERSPACE_START through @a NX_USERSPACE_END) should no longer be
 * used directly, since it is under the control of the memory
 * allocator.
 *
 * @sa nx_memalloc_init_full()
 */
void nx_memalloc_init(void);

/** Initialize a custom memory pool for the allocator.
 *
 * This is an explicit variant of nx_memalloc_init(), where you get to
 * specify what memory extent the allocator should control.
 *
 * @param mem_pool Pointer to the start of the memory pool.
 * @param mem_pool_size The size of the memory pool.
 *
 * @sa nx_memalloc_init()
 */
void nx_memalloc_init_full(void *mem_pool, U32 mem_pool_size);

/** Return the amount of memory used by the allocator.
 *
 * @return The amount of memory used, in bytes.
 *
 * @note The amount return includes both user-usable allocated memory
 * and TLSF overhead.
 */
U32 nx_memalloc_used(void);

/** Release control over the memory pool.
 *
 * Once destroyed, the allocator can no longer be used, and the caller
 * is free to do what he wants with the memory region originally handed
 * over to the allocator.
 */
void nx_memalloc_destroy(void);

/*@}*/

/** @name Allocating memory
 *
 * These functions behave as their libc equivalent would. They are
 * however placed in the NxOS namespace, so that application kernels can
 * link with a libc implementation without causing duplicate definition
 * errors.
 */
/*@{*/

/** Allocate and return a pointer to a block of @a size bytes.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated block on success, or NULL on failure.
 */
void *nx_malloc(U32 size);

/** Allocate and return a pointer to a zeroed set of @a nelem blocks of
 *  @a elem_size each.
 *
 * @param nelem Number of elements to allocate.
 * @param elem_size Length in bytes of one element.
 * @return A pointer to the allocated block on success, or NULL on failure.
 */
void *nx_calloc(U32 nelem, U32 elem_size);

/** Resize the memory block pointed to by @a ptr to @a size bytes.
 *
 * The block pointed to by @a ptr must have been previously allocated by
 * nx_malloc(), nx_calloc() or nx_realloc().
 *
 * @param ptr A pointer to the previously allocated block to resize.
 * @param size New length of the memory block, in bytes.
 * @return A pointer to the resized block on success, or NULL on failure.
 *
 * @note As with the standard libc @a realloc, calling nx_realloc() with a
 * NULL @a ptr is equivalent to nx_malloc(size), and calling with a zero @a
 * size is equivalent to nx_free(ptr).
 */
void *nx_realloc(void *ptr, U32 size);

/** Return allocated memory to the memory pool.
 *
 * @param ptr A pointer to a block previously returned by nx_malloc(),
 * nx_calloc() or nx_realloc().
 */
void nx_free(void *ptr);

/*@}*/

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_MEMALLOC_H__ */

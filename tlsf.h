/** @file tlsf.h
 *  @brief A memory allocator with real time guarantees
 */

/*
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.3.2
 *
 * Written by Miguel Masmano Tello <mimastel@doctor.upv.es>
 *
 * Thanks to Ismael Ripoll for his suggestions and reviews
 *
 * Copyright (C) 2007, 2006, 2005, 2004
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 *
 * Adapted to NxOS by David Anderson.
 */

#ifndef __NXOS_BASE_TLSF_H__
#define __NXOS_BASE_TLSF_H__

#include "base/types.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup memAllocator Memory allocation
 *
 * The NxOS baseplate optionally provides the TLSF memory allocator for
 * use by application kernels. The TLSF allocator performs all
 * allocation functions in constant (<tt>O(1)</tt>) time, and is
 * optimized to minimize fragmentation, making it ideal for low resource
 * embedded systems such as the NXT.
 *
 * No parts of the baseplate uses the allocator, so if you do not want
 * or need it, it will not be compiled into your application kernel.
 *
 * @note If you do want to use the allocator, you will first need to
 * initialize it by calling nx_mem_init(). All the other functions of
 * the allocator assume that the allocator is initialized.
 */
/*@{*/

/** @name Controlling the allocator */
/*@{*/

/** Initialize a memory pool for the allocator.
 *
 * The memory passed into this initialization function should no longer
 * be used directly, only through the allocator.
 *
 * @param mem_pool_size The size of the memory pool.
 * @param mem_pool Pointer to the start of the memory pool.
 * @return The amount of allocatable memory on success, or -1 on error.
 *
 * @note The usual way to call this function is
 * <tt>nx_mem_init(NX_USERSPACE_SIZE, NX_USERSPACE_START)</tt>, using
 * the symbols defined in memmap.h to hand over all of the memory
 * available to application kernels over to the allocator.
 */
size_t nx_mem_init(size_t mem_pool_size, void *mem_pool);

/** Return the number of bytes used by the allocator.
 *
 * The amount return includes both user-usable allocated memory and TLSF
 * overhead.
 */
size_t nx_mem_used();

/** Release control over the memory pool.
 *
 * Once destroyed, the allocator can no longer be used, and the caller
 * is free to do what he wants with the memory region originally handed
 * over to the allocator.
 */
void nx_mem_destroy();

/*@}*/


/** @name Allocating memory */
/*@{*/

/** Allocate and return a pointer to a block of @a size bytes.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated block on success, or NULL on failure.
 */
void *malloc(size_t size);

/** Allocate and return a pointer to a zeroed set of @a nelem blocks of
 *  @a elem_size each.
 *
 * @param nelem Number of elements to allocate.
 * @param elem_size Length in bytes of one element.
 * @return A pointer to the allocated block on success, or NULL on failure.
 */
void *calloc(size_t nelem, size_t elem_size);

/** Resize the memory block pointed to by @a ptr to @a size bytes.
 *
 * The block pointed to by @a ptr must have been previously allocated by
 * malloc(), calloc() or realloc().
 *
 * @param ptr A pointer to the previously allocated block to resize.
 * @param size New length of the memory block, in bytes.
 * @return A pointer to the resized block on success, or NULL on failure.
 *
 * @note As with the standard libc @a realloc, calling @a realloc with a
 * NULL @a ptr is equivalent to malloc(size), and calling with a zero @a
 * size is equivalent to free(ptr).
 */
void *realloc(void *ptr, size_t size);

/** Return allocated memory to the memory pool.
 *
 * @param ptr A pointer to a block previously returned by malloc(),
 * calloc() or realloc().
 */
void free(void *ptr);

/*@}*/
/*@}*/
/*@}*/

#endif /* __NXOS_BASE_TLSF_H__ */

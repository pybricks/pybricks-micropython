/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include <stdint.h>

#include "nxos/memmap.h"
#include "nxos/assert.h"
#include "nxos/util.h"

#include "nxos/lib/memalloc/memalloc.h"

/* This is really ugly and I should be taken out and shot for even doing
 * it. But as far as I can tell, GNU ld doesn't do link-time inlining,
 * so I do this nasty piece of work to "encourage" gcc towards inlining
 * the calls to TLSF.
 */
#include "nxos/lib/memalloc/_tlsf.h"
#define printf(fmt, ...) /* Nothing, we don't printf. */
#include "nxos/lib/memalloc/_tlsf.c.inc"

inline void nx_memalloc_init_full(void *mem_pool, uint32_t mem_pool_size) {
  size_t size = init_memory_pool(mem_pool_size, mem_pool);
  NX_ASSERT_MSG(size > 0, "Failed to init\nmemory allocator");
}

void nx_memalloc_init(void) {
  nx_memalloc_init_full(NX_USERSPACE_START, NX_USERSPACE_SIZE);
}

uint32_t nx_memalloc_used(void) {
  return get_used_size(mp);
}

void nx_memalloc_destroy(void) {
  destroy_memory_pool(mp);
}

void *nx_malloc(uint32_t size) {
  void *ret = malloc_ex(size, mp);
  NX_ASSERT_MSG(ret != NULL, "Out of memory");
  return ret;
}

void *nx_calloc(uint32_t nelem, uint32_t elem_size) {
  void *ret = calloc_ex(nelem, elem_size, mp);
  NX_ASSERT_MSG(ret != NULL, "Out of memory");
  return ret;
}

void *nx_realloc(void *ptr, uint32_t size) {
  void *ret = realloc_ex(ptr, size, mp);
  NX_ASSERT_MSG(ret != NULL, "Out of memory");
  return ret;
}

void nx_free(void *ptr) {
  free_ex(ptr, mp);
}

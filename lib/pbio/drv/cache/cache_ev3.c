// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CACHE_EV3

#include <stddef.h>
#include <stdint.h>

#include <tiam1808/armv5/cp15.h>

#include <pbdrv/compiler.h>

void pbdrv_cache_prepare_before_dma(const void *buf, size_t sz) {
    // Make sure all data is written by the compiler...
    pbdrv_compiler_memory_barrier();
    // and then make sure it's written out of the cache...
    CP15DCacheCleanBuff((uint32_t)buf, sz);
    // and also the write buffer, in case the cache missed.
    CP15DrainWriteBuffer();
}

void pbdrv_cache_prepare_after_dma(const void *buf, size_t sz) {
    // Invalidate stale data in the cache...
    CP15DCacheFlushBuff((uint32_t)buf, sz);
    // and then make sure _subsequent_ reads cannot be reordered earlier.
    pbdrv_compiler_memory_barrier();
}

#endif // PBDRV_CONFIG_CACHE_EV3

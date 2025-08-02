// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _PBDRV_CACHE_H_

#include <stddef.h>

// Gets data ready so that the data that we (the CPU) have written
// can be read by DMA peripherals. This cleans the relevant cache lines
// and flushes the write buffer.
void pbdrv_cache_prepare_before_dma(const void *buf, size_t sz);

// Makes sure that we (the CPU) are able to read data written
// by DMA peripherals. This invalidates the relevant cache lines.
void pbdrv_cache_prepare_after_dma(const void *buf, size_t sz);

// Accesses a variable via the uncached memory alias
#define PBDRV_UNCACHED(x)           (*(volatile __typeof__(x) *)((uint32_t)(&(x)) + 0x10000000))

// Gets the address of the uncached memory alias of a variable
#define PBDRV_UNCACHED_ADDR(x)      ((__typeof__(x) *)((uint32_t)(&(x)) + 0x10000000))

// Cache line size
#define PBDRV_CACHE_LINE_SZ         32

// Align data to a cache line, which is needed for clean RX DMA
#define PBDRV_DMA_BUF               __attribute__((aligned(PBDRV_CACHE_LINE_SZ), section(".dma")))

#endif

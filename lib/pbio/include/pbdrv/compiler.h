// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _PBDRV_COMPILER_H_

// Marks a switch case that intentionally falls through to the next one
#define PBDRV_FALL_THROUGH                  __attribute__((fallthrough))

// Forces the compiler to not reorder memory access around this line
#define pbdrv_compiler_memory_barrier()     __asm__ volatile ("" ::: "memory")

#endif

// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2023 The Pybricks Authors

#include <stdint.h>

static inline uint32_t swp(uint32_t *dest, uint32_t val) {
    uint32_t ret;
    __asm__ ("swp %0, %2, [%1]" : "=&r" (ret) : "r" (dest), "r" (val) : "memory");
    return ret;
}

static inline uint32_t swpb(uint8_t *dest, uint32_t val) {
    uint32_t ret;
    __asm__ ("swpb %0, %2, [%1]" : "=&r" (ret) : "r" (dest), "r" (val) : "memory");
    return ret;
}

// These attributes are required to make thumb interwork work properly,
// otherwise things get optimized out and we will try to call arm instructions
// from thumb code and get a crash.

__attribute__((noinline))
__attribute__((target("arm")))
uint32_t nx_atomic_cas32(uint32_t *dest, uint32_t val) {
    return swp(dest, val);
}

__attribute__((noinline))
__attribute__((target("arm")))
uint8_t nx_atomic_cas8(uint8_t *dest, uint8_t val) {
    return swpb(dest, val);
}

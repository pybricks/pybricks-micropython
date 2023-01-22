// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2023 The Pybricks Authors

#include <stdint.h>

#include "nxos/asm_decls.h"

static inline uint32_t get_cpsr(void) {
    uint32_t ret;
    __asm__ ("mrs %0, cpsr" : "=r"(ret));
    return ret;
}

static inline void set_cpsr(uint32_t state) {
    __asm__ ("msr cpsr, %0" : : "r"(state));
}

// These attributes are required to make thumb interwork work properly,
// otherwise things get optimized out and we will try to call arm instructions
// from thumb code and get a crash.

__attribute__((noinline))
__attribute__((target("arm")))
uint32_t nx_interrupts_disable(void) {
    uint32_t state = get_cpsr();
    set_cpsr(state | IRQ_FIQ_MASK);
    return state;
}

__attribute__((noinline))
__attribute__((target("arm")))
void nx_interrupts_enable(uint32_t state) {
    uint32_t old = get_cpsr();
    set_cpsr((old & ~IRQ_FIQ_MASK) | (state & IRQ_FIQ_MASK));
}

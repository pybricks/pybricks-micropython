// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Compact replacements for the AEABI integer division helpers.
//
// Cortex-M0 has no divide instruction, so the compiler turns every 32-bit
// integer division into a call to one of the helpers below. Since GCC 11, the
// v6-m libgcc only ships speed-optimized versions: __udivsi3 is fully
// unrolled and __divsi3 contains its own second copy of the unsigned core
// instead of sharing it, which together take around 730 bytes. The loop-based
// versions here produce identical results in around 110 bytes.
//
// The cost is speed: roughly 3 to 5 times slower per division. The heaviest
// user is motor control with around 10 divisions per 5 ms loop, which stays
// well below 1% of the CPU either way, in line with these hubs already
// trading speed for size elsewhere (CSUPEROPT = -Os).
//
// Defining these symbols in an object file makes the linker prefer them over
// the libgcc archive members. This file must be compiled without LTO: the
// compiler generates references to these helpers during LTO code generation,
// after LTO symbol resolution, so definitions inside an LTO object fail to
// link.
//
// Division by zero returns a quotient of 0 and a remainder equal to the
// dividend, the same as libgcc's default __aeabi_idiv0 behavior.

#include <stdint.h>

// The *divmod helpers return the quotient in r0 and the remainder in r1,
// which matches how a uint64_t is returned (low word in r0, high in r1).

uint64_t __aeabi_uidivmod(uint32_t n, uint32_t d) {
    if (d == 0) {
        return (uint64_t)n << 32;
    }

    // Scale the divisor up to the dividend, then do schoolbook long division
    // in base 2, one quotient bit per iteration.
    uint32_t q = 0;
    uint32_t bit = 1;
    while (d < n && !(d & UINT32_C(0x80000000))) {
        d <<= 1;
        bit <<= 1;
    }
    while (bit) {
        if (n >= d) {
            n -= d;
            q |= bit;
        }
        d >>= 1;
        bit >>= 1;
    }
    return ((uint64_t)n << 32) | q;
}

uint32_t __aeabi_uidiv(uint32_t n, uint32_t d) {
    return (uint32_t)__aeabi_uidivmod(n, d);
}
uint32_t __udivsi3(uint32_t n, uint32_t d) __attribute__((alias("__aeabi_uidiv")));

uint64_t __aeabi_idivmod(int32_t n, int32_t d) {
    uint64_t res = __aeabi_uidivmod(n < 0 ? -(uint32_t)n : (uint32_t)n, d < 0 ? -(uint32_t)d : (uint32_t)d);
    uint32_t q = (uint32_t)res;
    uint32_t r = (uint32_t)(res >> 32);

    // C99 division truncates toward zero: the quotient is negative when the
    // signs differ and the remainder takes the sign of the dividend.
    if ((n ^ d) < 0) {
        q = -q;
    }
    if (n < 0) {
        r = -r;
    }
    return ((uint64_t)r << 32) | q;
}

int32_t __aeabi_idiv(int32_t n, int32_t d) {
    return (int32_t)__aeabi_idivmod(n, d);
}
int32_t __divsi3(int32_t n, int32_t d) __attribute__((alias("__aeabi_idiv")));

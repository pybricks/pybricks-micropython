// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdbool.h>
#include <inttypes.h>
#include <fixmath.h>

int32_t pbio_math_sign(int32_t a) {
    if (a == 0) {
        return 0;
    }
    return a > 0 ? 1 : -1;
}

int32_t pbio_math_div_i32_fix16(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_div(fix16_from_int(a), b));
}

int32_t pbio_math_mul_i32_fix16(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_mul(fix16_from_int(a), b));
}

int32_t pbio_math_sqrt(int32_t n) {
    if (n <= 0) {
        return 0;
    }
    int32_t x0 = n;
    int32_t x1 = x0;

    while (true) {
        x1 = (x0 + n/x0)/2;
        if (x1 == x0 || x1 == x0 + 1) {
            return x0;
        }
        x0 = x1;
    }
}

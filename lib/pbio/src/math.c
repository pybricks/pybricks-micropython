// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#include <fixmath.h>

/**
 * Clamps a value to a +/- limit.
 *
 * If @p value is greater than positive @p abs_max, then positive @p abs_max is
 * returned. If @p value is less than negative @p abs_max, then negative
 * @p abs_max is returned. Otherwise @p value is returned.
 *
 * @param [in]  value   The value to clamp.
 * @param [in]  abs_max The clamp limit. This must be a positive value.
 * @return              The clamped value.
 */
int32_t pbio_math_clamp(int32_t value, int32_t abs_max) {
    assert(abs_max > 0);

    if (value > abs_max) {
        return abs_max;
    }
    if (value < -abs_max) {
        return -abs_max;
    }
    return value;
}

int32_t pbio_math_sign(int32_t a) {
    if (a == 0) {
        return 0;
    }
    return a > 0 ? 1 : -1;
}

int32_t pbio_math_mul_i32_fix16(int32_t a, fix16_t b) {
    int64_t product = (int64_t)a * b;
    if (product < 0) {
        product--;
    }

    int32_t result = product >> 16;
    result += (product & 0x8000) >> 15;

    return result;
}

int32_t pbio_math_div_i32_fix16(int32_t a, fix16_t b) {
    if (b == fix16_one) {
        return a;
    }
    return pbio_math_mul_i32_fix16(a, fix16_div(fix16_one, b));
}

int32_t pbio_math_sqrt(int32_t n) {
    if (n <= 0) {
        return 0;
    }
    int32_t x0 = n;
    int32_t x1 = x0;

    while (true) {
        x1 = (x0 + n / x0) / 2;
        if (x1 == x0 || x1 == x0 + 1) {
            return x0;
        }
        x0 = x1;
    }
}

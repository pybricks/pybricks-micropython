// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

/**
 * Gets the absolute value.
 *
 * @param [in]  value   The value.
 * @return              The absolute (positive) value.
 */
int32_t pbio_math_abs(int32_t value) {
    if (value < 0) {
        return -value;
    }
    return value;
}

/**
 * Binds a value between a lower and upper limit.
 *
 * If @p value is greater than @p max, then @p max is
 * returned. If @p value is less than @p min, then
 * @p min is returned. Otherwise @p value is returned.
 *
 * @param [in]  value   The value to bind.
 * @param [in]  min     The lower boundary value.
 * @param [in]  max     The upper boundary value.
 * @return              The bounded value.
 */
int32_t pbio_math_bind(int32_t value, int32_t min, int32_t max) {
    assert(max >= min);

    if (value > max) {
        return max;
    }
    if (value < min) {
        return min;
    }
    return value;
}

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

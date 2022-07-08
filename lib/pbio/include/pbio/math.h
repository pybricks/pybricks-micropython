// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup Math pbio: Integer math utilities
 *
 * Integer math utilities used by the pbio library.
 * @{
 */

#ifndef _PBIO_MATH_H_
#define _PBIO_MATH_H_

#include <stdint.h>

/**
 * Gets the absolute value.
 *
 * @param [in]  value   The value.
 * @return              The absolute (positive) value.
 */
static inline int32_t pbio_math_abs(int32_t value) {
    return __builtin_abs(value);
}

/**
 * Gets the maximum of two values.
 *
 * @param [in]  a       Value.
 * @param [in]  b       Value.
 * @return              a if it is greater than b, else b.
 */
static inline int32_t pbio_math_max(int32_t a, int32_t b) {
    if (a > b) {
        return a;
    }

    return b;
}

/**
 * Gets the minimum of two values.
 *
 * @param [in]  a       Value.
 * @param [in]  b       Value.
 * @return              a if it is less than b, else b.
 */
static inline int32_t pbio_math_min(int32_t a, int32_t b) {
    if (a < b) {
        return a;
    }

    return b;
}

/**
 * Get the sign of @p a.
 *
 * @param [in]  a   A signed integer value.
 * @return          1 if @p a is positive, -1 if @p a is negative or 0 if @p a
 *                  is 0.
 */
static inline int32_t pbio_math_sign(int32_t a) {
    if (a == 0) {
        return 0;
    }

    return a > 0 ? 1 : -1;
}

int32_t pbio_math_atan2(int32_t y, int32_t x);
int32_t pbio_math_bind(int32_t value, int32_t min, int32_t max);
int32_t pbio_math_clamp(int32_t value, int32_t abs_max);
int32_t pbio_math_sqrt(int32_t n);

#endif // _PBIO_MATH_H_

/** @} */

// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup Math pbio/math: Integer math utilities
 *
 * Integer math utilities used by the pbio library.
 * @{
 */

#ifndef _pbio_int_math_H_
#define _pbio_int_math_H_

#include <stdint.h>
#include <stdbool.h>

// Clamping and binding:

int32_t pbio_int_math_bind(int32_t value, int32_t min, int32_t max);
int32_t pbio_int_math_clamp(int32_t value, int32_t abs_max);
int32_t pbio_int_math_max(int32_t a, int32_t b);
int32_t pbio_int_math_min(int32_t a, int32_t b);

// Signing:

bool pbio_int_math_sign_not_opposite(int32_t a, int32_t b);
int32_t pbio_int_math_abs(int32_t value);
int32_t pbio_int_math_sign(int32_t a);

// Integer re-implementations of selected math functions.

int32_t pbio_int_math_atan2(int32_t y, int32_t x);
int32_t pbio_int_math_mult_then_div(int32_t a, int32_t b, int32_t c);
int32_t pbio_int_math_sqrt(int32_t n);

#endif // _pbio_int_math_H_

/** @} */

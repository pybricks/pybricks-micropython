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

int32_t pbio_math_abs(int32_t value);
int32_t pbio_math_atan2(int32_t y, int32_t x);
int32_t pbio_math_bind(int32_t value, int32_t min, int32_t max);
int32_t pbio_math_clamp(int32_t value, int32_t abs_max);
int32_t pbio_math_max(int32_t a, int32_t b);
int32_t pbio_math_min(int32_t a, int32_t b);
int32_t pbio_math_sign(int32_t a);
int32_t pbio_math_sqrt(int32_t n);

#endif // _PBIO_MATH_H_

/** @} */

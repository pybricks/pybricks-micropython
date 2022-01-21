// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_MATH_H_
#define _PBIO_MATH_H_

#include <stdint.h>

#include <fixmath.h>

int32_t pbio_math_clamp(int32_t value, int32_t abs_max);
int32_t pbio_math_sign(int32_t a);
int32_t pbio_math_div_i32_fix16(int32_t a, fix16_t b);
int32_t pbio_math_mul_i32_fix16(int32_t a, fix16_t b);
int32_t pbio_math_sqrt(int32_t n);

#endif // _PBIO_MATH_H_

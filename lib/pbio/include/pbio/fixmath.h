// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_FIXMATH_H_
#define _PBIO_FIXMATH_H_

#include <stdint.h>

#include <fixmath.h>

int32_t int_fix16_div(int32_t a, fix16_t b);
int32_t int_fix16_mul(int32_t a, fix16_t b);
int32_t int_sqrt(int32_t n);
int32_t int_abs(int32_t val);

#endif // _PBIO_FIXMATH_H_

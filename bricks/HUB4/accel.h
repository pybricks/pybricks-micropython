// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PYBRICKS_MOVEHUB_ACCEL_H_
#define _PYBRICKS_MOVEHUB_ACCEL_H_

#include <stdbool.h>

void accel_init(void);
void accel_get_values(int *x, int *y, int *z);
void accel_deinit(void);

#endif /* _PYBRICKS_MOVEHUB_ACCEL_H_ */

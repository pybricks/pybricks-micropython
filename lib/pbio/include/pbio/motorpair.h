// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_MOTORPAIR_H_
#define _PBIO_MOTORPAIR_H_

#include <pbio/motor.h>

typedef struct _pbio_motor_pair_t {
    pbio_motor_t *motor_one;
    pbio_motor_t *motor_two;
} pbio_motor_pair_t;

pbio_error_t pbio_get_motor_pair(pbio_motor_t *mtr1, pbio_motor_t *mtr2, pbio_motor_pair_t* pair);

#endif // _PBIO_MOTORPAIR_H_

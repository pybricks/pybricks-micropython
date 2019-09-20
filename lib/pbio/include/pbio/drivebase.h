// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_DRIVEBASE_H_
#define _PBIO_DRIVEBASE_H_

#include <pbio/servo.h>

typedef struct _pbio_motor_pair_t {
    pbio_servo_t *motor_one;
    pbio_servo_t *motor_two;
} pbio_motor_pair_t;

pbio_error_t pbio_get_motor_pair(pbio_servo_t *mtr1, pbio_servo_t *mtr2, pbio_motor_pair_t* pair);

#endif // _PBIO_DRIVEBASE_H_

// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_DRIVEBASE_H_
#define _PBIO_DRIVEBASE_H_

#include <pbio/servo.h>

typedef struct _pbio_motor_pair_t {
    pbio_servo_t *servo1;
    pbio_servo_t *servo2;
} pbio_motor_pair_t;

pbio_error_t pbio_get_motor_pair(pbio_servo_t *srv1, pbio_servo_t *srv2, pbio_motor_pair_t* pair);

#endif // _PBIO_DRIVEBASE_H_

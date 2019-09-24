// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_DRIVEBASE_H_
#define _PBIO_DRIVEBASE_H_

#include <pbio/servo.h>

typedef struct _pbio_drivebase_t {
    pbio_servo_t *left;
    pbio_servo_t *right;
} pbio_drivebase_t;

pbio_error_t pbio_drivebase_get(pbio_servo_t *left, pbio_servo_t *right, pbio_drivebase_t **pair);

#endif // _PBIO_DRIVEBASE_H_

// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk


#include <pbio/servo.h>
#include <pbio/drivebase.h>

static pbio_drivebase_t base;

static pbio_error_t pbio_drivebase_setup(pbio_drivebase_t *drivebase, pbio_servo_t *left, pbio_servo_t *right) {
    
    pbio_error_t err;

    // Reset both motors to a passive state
    err = pbio_servo_stop(left, PBIO_MOTOR_STOP_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_stop(right, PBIO_MOTOR_STOP_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Individual servos
    drivebase->left = left;
    drivebase->right = right;

    // Drivebase geometry

    return PBIO_SUCCESS;
}


pbio_error_t pbio_drivebase_get(pbio_drivebase_t **drivebase, pbio_servo_t *left, pbio_servo_t *right) {
    // Unique pointer to drivebase device
    *drivebase = &base;

    // Configure drivebase and set properties
    return pbio_drivebase_setup(*drivebase, left, right);
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

// Number of resolution counts per degree and mm drivebase motion
#define COUNTS_PER_DEGREE (10)
#define COUNTS_PER_MM (10)

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_drivebase_t __db;

static pbio_error_t pbio_drivebase_setup(pbio_drivebase_t *db,
                                         pbio_servo_t *left,
                                         pbio_servo_t *right,
                                         fix16_t wheel_diameter,
                                         fix16_t axle_track) {
    pbio_error_t err;

    // Reset both motors to a passive state
    err = pbio_servo_stop(left, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_stop(right, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Individual servos
    db->left = left;
    db->right = right;

    // Drivebase geometry
    if (wheel_diameter <= 0 || axle_track <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    db->wheel_diameter = wheel_diameter;
    db->axle_track = axle_track;

    // Turn counts for every degree difference between the servo motors
    db->turn_counts_per_diff = fix16_div(
        fix16_mul(db->wheel_diameter, fix16_from_int(COUNTS_PER_DEGREE)),
        fix16_mul(db->axle_track, fix16_from_int(2))
    );

    // Forward drive counts for every summed degree of the servo motors
    db->drive_counts_per_sum = fix16_div(
        fix16_mul(fix16_mul(db->wheel_diameter, fix16_pi), fix16_from_int(COUNTS_PER_MM)),
        fix16_from_int(720)
    );

    // Claim servos
    db->left->state = PBIO_SERVO_STATE_CLAIMED;
    db->right->state = PBIO_SERVO_STATE_CLAIMED;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_get(pbio_drivebase_t **_db, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track) {

    // Get pointer to device
    pbio_drivebase_t *db = &__db;

    // Configure drivebase and set properties
    pbio_error_t err = pbio_drivebase_setup(db, left, right, wheel_diameter, axle_track);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // On success, return pointer to device
    *_db = db;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_actuation_t after_stop) {

    pbio_error_t err;

    switch (after_stop) {
        case PBIO_ACTUATION_COAST:
            // Stop by coasting
            err = pbio_hbridge_coast(db->left->hbridge);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            return pbio_hbridge_coast(db->right->hbridge);
        case PBIO_ACTUATION_BRAKE:
            // Stop by braking
            err = pbio_hbridge_brake(db->left->hbridge);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            return pbio_hbridge_brake(db->right->hbridge);
        default:
            // HOLD is not implemented
            return PBIO_ERROR_INVALID_ARG;
    }
}

pbio_error_t pbio_drivebase_start(pbio_drivebase_t *db, int32_t speed, int32_t rate) {

    pbio_error_t err;

    // FIXME: This is a fake drivebase without synchronization
    int32_t sum = 180 * pbio_math_mul_i32_fix16(pbio_math_div_i32_fix16(speed, db->wheel_diameter), FOUR_DIV_PI);
    int32_t dif = 2 * pbio_math_div_i32_fix16(pbio_math_mul_i32_fix16(rate, db->axle_track), db->wheel_diameter);

    err = pbio_hbridge_set_duty_cycle_sys(db->left->hbridge, ((sum+dif)/2)*10);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return pbio_hbridge_set_duty_cycle_sys(db->right->hbridge, ((sum-dif)/2)*10);
}

static pbio_error_t pbio_drivebase_update(pbio_drivebase_t *db) {
    return PBIO_SUCCESS;
}

// TODO: Convert to Contiki process

// Service all drivebase motors by calling this function at approximately constant intervals.
void _pbio_drivebase_poll(void) {
    pbio_drivebase_t *db = &__db;

    if (db->left && db->left->connected && db->right && db->right->connected) {
        pbio_drivebase_update(db);
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

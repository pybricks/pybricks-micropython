// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_DRIVEBASE_H_
#define _PBIO_DRIVEBASE_H_

#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

typedef struct _pbio_drivebase_t {
    pbio_servo_t *left;
    pbio_servo_t *right;
    int32_t sum_offset;
    int32_t dif_offset;
    pbio_control_t control_heading;
    pbio_control_t control_distance;
} pbio_drivebase_t;

pbio_error_t pbio_drivebase_setup(pbio_drivebase_t *db, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track);
pbio_error_t pbio_drivebase_update(pbio_drivebase_t *db);
void pbio_drivebase_claim_servos(pbio_drivebase_t *db, bool claim);

// Finite point to point control

pbio_error_t pbio_drivebase_curve(pbio_drivebase_t *db, int32_t radius, int32_t angle, int32_t drive_speed, int32_t turn_rate, pbio_actuation_t after_stop);

pbio_error_t pbio_drivebase_straight(pbio_drivebase_t *db, int32_t distance, int32_t straight_speed, pbio_actuation_t after_stop);

pbio_error_t pbio_drivebase_turn(pbio_drivebase_t *db, int32_t angle, int32_t turn_rate, pbio_actuation_t after_stop);

// Infinite driving

pbio_error_t pbio_drivebase_drive(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate);

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_actuation_t after_stop);

void pbio_drivebase_stop_control(pbio_drivebase_t *db);

// Measuring

pbio_error_t pbio_drivebase_get_state_user(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate);

pbio_error_t pbio_drivebase_reset_state(pbio_drivebase_t *db);

// Settings

pbio_error_t pbio_drivebase_get_drive_settings(pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *turn_rate, int32_t *turn_acceleration);

pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t turn_rate, int32_t turn_acceleration);

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

#endif // _PBIO_DRIVEBASE_H_

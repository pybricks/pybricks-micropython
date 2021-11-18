// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_DRIVEBASE_H_
#define _PBIO_DRIVEBASE_H_

#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

#define PBIO_RADIUS_INF (INT32_MAX)

typedef struct _pbio_drivebase_t {
    pbio_servo_t *left;
    pbio_servo_t *right;
    pbio_control_t control_heading;
    pbio_control_t control_distance;
} pbio_drivebase_t;

pbio_error_t pbio_drivebase_setup(pbio_drivebase_t *db, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track);
pbio_error_t pbio_drivebase_update(pbio_drivebase_t *db);
void pbio_drivebase_claim_servos(pbio_drivebase_t *db, bool claim);

// Finite point to point control

pbio_error_t pbio_drivebase_drive_curve(pbio_drivebase_t *db, int32_t radius, int32_t angle_or_distance, int32_t drive_speed, int32_t turn_rate, pbio_actuation_t after_stop);

// Infinite driving

pbio_error_t pbio_drivebase_drive_forever(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate);

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_actuation_t after_stop);

void pbio_drivebase_stop_control(pbio_drivebase_t *db);

bool pbio_drivebase_is_busy(pbio_drivebase_t *db);

// Measuring and settings

pbio_error_t pbio_drivebase_get_state_user(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate);

pbio_error_t pbio_drivebase_get_drive_settings(pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *turn_rate, int32_t *turn_acceleration);

pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t turn_rate, int32_t turn_acceleration);

#if !PBIO_CONFIG_CONTROL_MINIMAL

// SPIKE drive base wrappers

pbio_error_t pbio_spikebase_setup(pbio_drivebase_t *db, pbio_servo_t *servo_left, pbio_servo_t *servo_right);

pbio_error_t pbio_spikebase_drive_forever(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right);

pbio_error_t pbio_spikebase_drive_time(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t duration, pbio_actuation_t after_stop);

pbio_error_t pbio_spikebase_drive_angle(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t angle, pbio_actuation_t after_stop);

pbio_error_t pbio_spikebase_steering_to_tank(int32_t speed, int32_t steering, int32_t *speed_left, int32_t *speed_right);

#endif // !PBIO_CONFIG_CONTROL_MINIMAL

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

#endif // _PBIO_DRIVEBASE_H_

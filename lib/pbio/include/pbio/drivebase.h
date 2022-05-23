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

pbio_error_t pbio_drivebase_get_drivebase(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track);

void pbio_drivebase_update_all(void);
bool pbio_drivebase_update_loop_is_running(pbio_drivebase_t *db);

// Finite point to point control

pbio_error_t pbio_drivebase_drive_curve(pbio_drivebase_t *db, int32_t radius, int32_t angle_or_distance, pbio_control_on_completion_t on_completion);

// Infinite driving

pbio_error_t pbio_drivebase_drive_forever(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate);

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_dcmotor_actuation_t actuation);

bool pbio_drivebase_is_busy(pbio_drivebase_t *db);

// Measuring and settings

pbio_error_t pbio_drivebase_get_state_user(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate);

pbio_error_t pbio_drivebase_get_drive_settings(pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *drive_deceleration, int32_t *turn_rate, int32_t *turn_acceleration, int32_t *turn_deceleration);

pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t drive_deceleration, int32_t turn_rate, int32_t turn_acceleration, int32_t turn_deceleration);

#if !PBIO_CONFIG_CONTROL_MINIMAL

// SPIKE drive base wrappers

pbio_error_t pbio_drivebase_get_spikebase(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right);

pbio_error_t pbio_spikebase_drive_forever(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right);

pbio_error_t pbio_spikebase_drive_time(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t duration, pbio_control_on_completion_t on_completion);

pbio_error_t pbio_spikebase_drive_angle(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t angle, pbio_control_on_completion_t on_completion);

pbio_error_t pbio_spikebase_steering_to_tank(int32_t speed, int32_t steering, int32_t *speed_left, int32_t *speed_right);

#endif // !PBIO_CONFIG_CONTROL_MINIMAL

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

#endif // _PBIO_DRIVEBASE_H_

// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

/**
 * @addtogroup Drivebase pbio/drivebase: Drivebase motion control
 *
 * Precision driving and steering using odometry.
 * @{
 */

#ifndef _PBIO_DRIVEBASE_H_
#define _PBIO_DRIVEBASE_H_

#include <pbio/servo.h>

#include <pbio/imu.h>

#if PBIO_CONFIG_NUM_DRIVEBASES > 0

typedef struct _pbio_drivebase_t {
    /**
     * Whether to use the gyro for heading control, and if so which type.
     */
    pbio_imu_heading_type_t gyro_heading_type;
    /**
     * Synchronization state to indicate that one or more controllers are paused.
     */
    bool control_paused;
    pbio_servo_t *left;
    pbio_servo_t *right;
    /**
     * Offset of the reported heading angle with respect to the measured value.
     *
     * Not used if gyro control is active. Then the gyro module handles offset.
     */
    pbio_angle_t heading_offset;
    /**
     * Heading controller.
     */
    pbio_control_t control_heading;
    /**
     * Offset of the reported distance with respect to the measured value.
     */
    pbio_angle_t distance_offset;
    /**
     * Distance controller.
     */
    pbio_control_t control_distance;
} pbio_drivebase_t;

pbio_error_t pbio_drivebase_get_drivebase(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right, int32_t wheel_diameter, int32_t axle_track);

// Drive base status:

void pbio_drivebase_update_all(void);
bool pbio_drivebase_update_loop_is_running(pbio_drivebase_t *db);
bool pbio_drivebase_any_uses_gyro(void);
bool pbio_drivebase_is_done(const pbio_drivebase_t *db);
pbio_error_t pbio_drivebase_is_stalled(pbio_drivebase_t *db, bool *stalled, uint32_t *stall_duration);

// Finite point to point control:

pbio_error_t pbio_drivebase_drive_straight(pbio_drivebase_t *db, int32_t distance, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_drivebase_drive_curve(pbio_drivebase_t *db, int32_t radius, int32_t angle, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_drivebase_drive_arc_angle(pbio_drivebase_t *db, int32_t radius, int32_t angle, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_drivebase_drive_arc_distance(pbio_drivebase_t *db, int32_t radius, int32_t distance, pbio_control_on_completion_t on_completion);

// Infinite driving:

pbio_error_t pbio_drivebase_drive_forever(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate);
pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_control_on_completion_t on_completion);


// Measuring and settings:

pbio_error_t pbio_drivebase_get_state_user(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate);
pbio_error_t pbio_drivebase_get_state_user_angle(pbio_drivebase_t *db, float *angle);
pbio_error_t pbio_drivebase_reset(pbio_drivebase_t *db, int32_t distance, int32_t angle);
pbio_error_t pbio_drivebase_get_drive_settings(const pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *drive_deceleration, int32_t *turn_rate, int32_t *turn_acceleration, int32_t *turn_deceleration);
pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t drive_deceleration, int32_t turn_rate, int32_t turn_acceleration, int32_t turn_deceleration);
pbio_error_t pbio_drivebase_set_use_gyro(pbio_drivebase_t *db, pbio_imu_heading_type_t heading_type);

#if PBIO_CONFIG_DRIVEBASE_SPIKE

// SPIKE drive base wrappers:

pbio_error_t pbio_drivebase_get_drivebase_spike(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right);
pbio_error_t pbio_drivebase_spike_drive_forever(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right);
pbio_error_t pbio_drivebase_spike_drive_time(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, uint32_t duration, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_drivebase_spike_drive_angle(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t angle, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_drivebase_spike_steering_to_tank(int32_t speed, int32_t steering, int32_t *speed_left, int32_t *speed_right);

#endif // PBIO_CONFIG_DRIVEBASE_SPIKE

#endif // PBIO_CONFIG_NUM_DRIVEBASES > 0

#endif // _PBIO_DRIVEBASE_H_

/** @} */

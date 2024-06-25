// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

/**
 * @addtogroup Imu Imu functions
 *
 * Provides functions for accessing hub imu.
 * @{
 */

#ifndef _PBIO_IMU_H_
#define _PBIO_IMU_H_

#include <stdint.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/geometry.h>

#if PBIO_CONFIG_IMU

void pbio_imu_init(void);

pbio_error_t pbio_imu_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis);

bool pbio_imu_is_stationary(void);

bool pbio_imu_is_ready(void);

void pbio_imu_get_settings(float *angular_velocity, float *acceleration, float *heading_correction);

pbio_error_t pbio_imu_set_settings(float angular_velocity, float acceleration, float heading_correction);

void pbio_imu_get_angular_velocity(pbio_geometry_xyz_t *values);

void pbio_imu_get_acceleration(pbio_geometry_xyz_t *values);

pbio_error_t pbio_imu_get_single_axis_rotation(pbio_geometry_xyz_t *axis, float *angle);

pbio_geometry_side_t pbio_imu_get_up_side(void);

float pbio_imu_get_heading(void);

void pbio_imu_set_heading(float desired_heading);

void pbio_imu_get_heading_scaled(pbio_angle_t *heading, int32_t *heading_rate, int32_t ctl_steps_per_degree);

#else // PBIO_CONFIG_IMU

static inline void pbio_imu_init(void) {
}

static inline pbio_error_t pbio_imu_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline bool pbio_imu_is_stationary(void) {
    return false;
}

static inline void pbio_imu_get_settings(float *angular_velocity, float *acceleration, float *heading_correction) {
}

static inline pbio_error_t pbio_imu_set_settings(float angular_velocity, float acceleration, float heading_correction) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_imu_get_angular_velocity(pbio_geometry_xyz_t *values) {
}

static inline void pbio_imu_get_acceleration(pbio_geometry_xyz_t *values) {
}

static inline pbio_geometry_side_t pbio_imu_get_up_side(void) {
    return PBIO_GEOMETRY_SIDE_TOP;
}

static inline float pbio_imu_get_heading(void) {
    return 0.0f;
}

static inline void pbio_imu_set_heading(float desired_heading) {
}

static inline void pbio_imu_get_heading_scaled(pbio_angle_t *heading, int32_t *heading_rate, int32_t ctl_steps_per_degree) {
}

#endif // PBIO_CONFIG_IMU

#endif // _PBIO_IMU_H_

/** @} */

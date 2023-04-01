// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup Orientation Orientation functions
 *
 * Provides functions for accessing hub orientation.
 * @{
 */

#ifndef _PBIO_ORIENTATION_H_
#define _PBIO_ORIENTATION_H_

#include <stdint.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/geometry.h>

#include <pbdrv/imu.h>

#if PBIO_CONFIG_IMU

void pbio_imu_init(void);

pbio_error_t pbio_orientation_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis);

bool pbio_imu_is_stationary(void);

void pbio_imu_set_stationary_thresholds(float angular_velocity, float acceleration);

void pbio_imu_get_angular_velocity(pbio_geometry_xyz_t *values);

void pbio_imu_get_acceleration(pbio_geometry_xyz_t *values);

pbio_geometry_side_t pbio_imu_get_up_side(void);

float pbio_imu_get_heading(void);

void pbio_imu_set_heading(float desired_heading);

void pbio_imu_get_heading_scaled(pbio_angle_t *heading, int32_t ctl_steps_per_degree);

#else // PBIO_CONFIG_IMU

static inline void pbio_imu_init(void) {
}

static inline pbio_error_t pbio_orientation_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline bool pbio_imu_is_stationary(void) {
    return false;
}

static inline void pbio_imu_set_stationary_thresholds(float angular_velocity, float acceleration) {
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

static inline void pbio_imu_get_heading_scaled(pbio_angle_t *heading, int32_t ctl_steps_per_degree) {
}

#endif // PBIO_CONFIG_IMU

#endif // _PBIO_ORIENTATION_H_

/** @} */

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

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/geometry.h>

#include <pbdrv/imu.h>

/**
 * Identifier for one side of a rectangle (e.g. screen) or box (e.g. a hub).
 */
typedef enum {
    PBIO_ORIENTATION_SIDE_FRONT =  (0 << 2) | 0,  /**< +X: The front side of a rectangular box */
    PBIO_ORIENTATION_SIDE_LEFT =   (0 << 2) | 1,  /**< +Y: The left side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_TOP =    (0 << 2) | 2,  /**< +Z: The top side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_BACK =   (1 << 2) | 0,  /**< -X: The back side of a rectangular box */
    PBIO_ORIENTATION_SIDE_RIGHT =  (1 << 2) | 1,  /**< -Y: The right side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_BOTTOM = (1 << 2) | 2,  /**< -Z: The bottom side of a rectangular box or screen */
} pbio_orientation_side_t;

void pbio_orientation_side_get_axis(pbio_orientation_side_t side, uint8_t *index, int8_t *sign);

void pbio_orientation_get_complementary_axis(uint8_t *index, int8_t *sign);

pbio_orientation_side_t pbio_orientation_side_from_vector(pbio_geometry_xyz_t *vector);

#if PBIO_CONFIG_ORIENTATION_IMU

void pbio_orientation_imu_init(void);

pbio_error_t pbio_orientation_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis);

bool pbio_orientation_imu_is_stationary(void);

void pbio_orientation_imu_set_stationary_thresholds(float angular_velocity, float acceleration);

void pbio_orientation_imu_get_angular_velocity(pbio_geometry_xyz_t *values);

void pbio_orientation_imu_get_acceleration(pbio_geometry_xyz_t *values);

pbio_orientation_side_t pbio_orientation_imu_get_up_side(void);

float pbio_orientation_imu_get_heading(void);

void pbio_orientation_imu_set_heading(float desired_heading);

#else // PBIO_CONFIG_ORIENTATION_IMU

static inline void pbio_orientation_imu_init(void) {
}

static inline pbio_error_t pbio_orientation_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline bool pbio_orientation_imu_is_stationary(void) {
    return false;
}

static inline void pbio_orientation_imu_set_stationary_thresholds(float angular_velocity, float acceleration) {
}

static inline void pbio_orientation_imu_get_angular_velocity(pbio_geometry_xyz_t *values) {
}

static inline void pbio_orientation_imu_get_acceleration(pbio_geometry_xyz_t *values) {
}

static inline pbio_orientation_side_t pbio_orientation_imu_get_up_side(void) {
    return PBIO_ORIENTATION_SIDE_TOP;
}

static inline float pbio_orientation_imu_get_heading(void) {
    return 0.0f;
}

static inline void pbio_orientation_imu_set_heading(float desired_heading) {
}

#endif // PBIO_CONFIG_ORIENTATION_IMU

#endif // _PBIO_ORIENTATION_H_

/** @} */

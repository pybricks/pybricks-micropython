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

/**
 * IMU settings flags.
 *
 * Note: Add new flags such that false is the default value.
 */
typedef enum {
    /**
     * The gyro stationary threshold has been updated.
     */
    PBIO_IMU_SETTINGS_FLAGS_GYRO_STATIONARY_THRESHOLD_SET = (1 << 0),
    /**
     * The accelerometer stationary threshold has been updated.
     */
    PBIO_IMU_SETTINGS_FLAGS_ACCEL_STATIONARY_THRESHOLD_SET = (1 << 1),
    /**
     * The initial values for the gyro bias have been set.
     */
    PBIO_IMU_SETTINGS_FLAGS_GYRO_BIAS_INITIAL_SET = (1 << 3),
    /**
     * The per-axis angular velocity scale has been set.
     */
    PBIO_IMU_SETTINGS_FLAGS_GYRO_SCALE_SET = (1 << 4),
    /**
     * The accelerometer offsets and scale has been calibrated.
     */
    PBIO_IMU_SETTINGS_FLAGS_ACCEL_CALIBRATED = (1 << 5),
} pbio_imu_persistent_settings_flags_t;

/**
 * Persistent IMU settings. All data types are little-endian.
 */
typedef struct {
    /**
     * Flags indicating which persistent settings have been updated by
     * the user or a calibration routine. In settings setter functions, this
     * flag value is used to indicate which values are being set.
     */
    uint32_t flags;
    /** Angular velocity threshold below which the IMU is considered stationary, in deg/s. */
    float gyro_stationary_threshold;
    /** Acceleration threshold below which the IMU is considered stationary, in mm/s^2. */
    float accel_stationary_threshold;
    /** Positive acceleration values */
    pbio_geometry_xyz_t gravity_pos;
    /** Negative acceleration values */
    pbio_geometry_xyz_t gravity_neg;
    /** Angular velocity stationary bias initially used on boot */
    pbio_geometry_xyz_t angular_velocity_bias_start;
    /** Angular velocity scale (unadjusted measured degrees per whole rotation) */
    pbio_geometry_xyz_t angular_velocity_scale;
} pbio_imu_persistent_settings_t;

#if PBIO_CONFIG_IMU

void pbio_imu_init(void);

void pbio_imu_set_default_settings(pbio_imu_persistent_settings_t *settings);

void pbio_imu_apply_loaded_settings(pbio_imu_persistent_settings_t *settings);

pbio_error_t pbio_imu_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis);

bool pbio_imu_is_stationary(void);

bool pbio_imu_is_ready(void);

pbio_error_t pbio_imu_get_settings(pbio_imu_persistent_settings_t **settings);

pbio_error_t pbio_imu_set_settings(pbio_imu_persistent_settings_t *new_settings);

void pbio_imu_get_angular_velocity(pbio_geometry_xyz_t *values, bool calibrated);

void pbio_imu_get_acceleration(pbio_geometry_xyz_t *values, bool calibrated);

pbio_error_t pbio_imu_get_single_axis_rotation(pbio_geometry_xyz_t *axis, float *angle);

pbio_geometry_side_t pbio_imu_get_up_side(bool calibrated);

float pbio_imu_get_heading(void);

void pbio_imu_set_heading(float desired_heading);

void pbio_imu_get_heading_scaled(pbio_angle_t *heading, int32_t *heading_rate, int32_t ctl_steps_per_degree);

#else // PBIO_CONFIG_IMU

static inline void pbio_imu_init(void) {
}

static inline void pbio_imu_set_default_settings(pbio_imu_persistent_settings_t *settings) {
}

static inline void pbio_imu_apply_loaded_settings(pbio_imu_persistent_settings_t *settings) {
}

static inline pbio_error_t pbio_imu_set_base_orientation(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

static inline bool pbio_imu_is_stationary(void) {
    return false;
}

static inline pbio_error_t pbio_imu_get_settings(pbio_imu_persistent_settings_t **settings) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_imu_set_settings(float angular_velocity, float acceleration, float heading_correction, bool request_save) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_imu_get_angular_velocity(pbio_geometry_xyz_t *values, bool calibrated) {
}

static inline void pbio_imu_get_acceleration(pbio_geometry_xyz_t *values, bool calibrated) {
}

static inline pbio_geometry_side_t pbio_imu_get_up_side(bool calibrated) {
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

// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <pbdrv/clock.h>
#include <pbdrv/imu.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/geometry.h>
#include <pbio/imu.h>
#include <pbio/int_math.h>
#include <pbio/util.h>

#if PBIO_CONFIG_IMU

static pbdrv_imu_dev_t *imu_dev;
static pbdrv_imu_config_t *imu_config;

// Cached sensor values that can be read at any time without polling again.
static pbio_geometry_xyz_t angular_velocity; // deg/s, in hub frame, already adjusted for bias.
static pbio_geometry_xyz_t acceleration; // mm/s^2, in hub frame
static pbio_geometry_xyz_t gyro_bias;
static pbio_geometry_xyz_t single_axis_rotation; // deg, in hub frame

// Called by driver to process one frame of unfiltered gyro and accelerometer data.
static void pbio_imu_handle_frame_data_func(int16_t *data) {
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(angular_velocity.values); i++) {
        // Update angular velocity and acceleration cache so user can read them.
        angular_velocity.values[i] = data[i] * imu_config->gyro_scale - gyro_bias.values[i];
        acceleration.values[i] = data[i + 3] * imu_config->accel_scale;

        // Update "heading" on all axes. This is not useful for 3D attitude
        // estimation, but it allows the user to get a 1D heading even with
        // the hub mounted at an arbitrary orientation. Such a 1D heading
        // is numerically more accurate, which is useful in drive base
        // applications so long as the vehicle drives on a flat surface.
        single_axis_rotation.values[i] += angular_velocity.values[i] * imu_config->sample_time;
    }
}

// This counter is a measure for calibration accuracy, roughly equivalent
// to the accumulative number of seconds it has been stationary in total.
static uint32_t stationary_counter = 0;
static uint32_t stationary_time_last;

/*
 * Tests if the imu is ready for use in a user program.
 *
 * @return    True if it has been stationary at least once in the last 10 minutes.
*/
bool pbio_imu_is_ready(void) {
    return stationary_counter > 0 && pbdrv_clock_get_ms() - stationary_time_last < 10 * 60 * 1000;
}

// Called by driver to process unfiltered gyro and accelerometer data recorded while stationary.
static void pbio_imu_handle_stationary_data_func(const int32_t *gyro_data_sum, const int32_t *accel_data_sum, uint32_t num_samples) {

    // If the IMU calibration hasn't been updated in a long time, reset the
    // stationary counter so that the calibration values get a large weight.
    if (!pbio_imu_is_ready()) {
        stationary_counter = 0;
    }

    stationary_time_last = pbdrv_clock_get_ms();
    stationary_counter++;

    // The relative weight of the new data in order to build a long term
    // average of the data without maintaining a data buffer.
    float weight = stationary_counter >= 20 ? 0.05f : 1.0f / stationary_counter;

    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(gyro_bias.values); i++) {
        // Average gyro rate while stationary, indicating current bias.
        float average_now = gyro_data_sum[i] * imu_config->gyro_scale / num_samples;

        // Update bias at decreasing rate.
        gyro_bias.values[i] = gyro_bias.values[i] * (1.0f - weight) + weight * average_now;
    }
}

/**
 * Initializes global imu module.
 */
void pbio_imu_init(void) {
    pbio_error_t err = pbdrv_imu_get_imu(&imu_dev, &imu_config);
    if (err != PBIO_SUCCESS) {
        return;
    }
    pbdrv_imu_set_data_handlers(imu_dev, pbio_imu_handle_frame_data_func, pbio_imu_handle_stationary_data_func);
}

/**
 * The "neutral" base orientation of the hub, describing how it is mounted
 * in the robot. All getters (tilt, acceleration, rotation, etc) give results
 * relative to this base orientation. Initial orientation is identity, hub flat.
 */
static pbio_geometry_matrix_3x3_t pbio_orientation_base_orientation = {
    .m11 = 1.0f, .m12 = 0.0f, .m13 = 0.0f,
    .m21 = 0.0f, .m22 = 1.0f, .m23 = 0.0f,
    .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f,
};

/**
 * Sets the hub base orientation.
 *
 * @param [in]  front_side_axis  Which way the hub front side points when it is
 *                               in the base orientation.
 * @param [in]  top_side_axis    Which way the hub top side points when it is
 *                               in the base orientation.
 * @return                       ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_ARG for incorrect axis values.
 */
pbio_error_t pbio_imu_set_base_orientation(pbio_geometry_xyz_t *front_side_axis, pbio_geometry_xyz_t *top_side_axis) {

    pbio_error_t err = pbio_geometry_map_from_base_axes(front_side_axis, top_side_axis, &pbio_orientation_base_orientation);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    pbio_imu_set_heading(0.0f);

    return PBIO_SUCCESS;
}

/**
 * Checks if the IMU is currently stationary.
 *
 * @return    True if it has been stationary for about a second, false if moving.
 */
bool pbio_imu_is_stationary(void) {
    return pbdrv_imu_is_stationary(imu_dev);
}

// Measured rotation of the Z axis in the user frame for exactly one rotation
// of the hub. This will be used to adjust the heading value, which is slightly
// different for each hub.
static float heading_degrees_per_rotation = 360.0f;

/**
 * Sets the IMU settings. This includes the thresholds that define when the hub
 * is stationary. When the measurements are steadily below these levels, the
 * orientation module automatically recalibrates. Also includes the hub-specific
 * correction value to get a more accurate heading value.
 *
 * If a value is nan, it is ignored.
 *
 * @param [in]  angular_velocity    Angular velocity threshold in deg/s.
 * @param [in]  acceleration        Acceleration threshold in mm/s^2
 * @param [in]  heading_correction  Measured degrees per full rotation of the hub.
 * @returns ::PBIO_ERROR_INVALID_ARG if the heading correction is out of range,
 *          otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_imu_set_settings(float angular_velocity, float acceleration, float heading_correction) {
    if (!isnan(angular_velocity)) {
        imu_config->gyro_stationary_threshold = pbio_int_math_bind(angular_velocity / imu_config->gyro_scale, 1, INT16_MAX);
    }
    if (!isnan(acceleration)) {
        imu_config->accel_stationary_threshold = pbio_int_math_bind(acceleration / imu_config->accel_scale, 1, INT16_MAX);
    }
    if (!isnan(heading_correction)) {
        if (heading_correction < 350 || heading_correction > 370) {
            return PBIO_ERROR_INVALID_ARG;
        }
        heading_degrees_per_rotation = heading_correction;
    }
    return PBIO_SUCCESS;
}

/**
 * Gets the thresholds that define when the hub is stationary.
 *
 * @param [out]  angular_velocity    Angular velocity threshold in deg/s.
 * @param [out]  acceleration        Acceleration threshold in mm/s^2
 * @param [out]  heading_correction  Measured degrees per full rotation of the hub.
 */
void pbio_imu_get_settings(float *angular_velocity, float *acceleration, float *heading_correction) {
    *angular_velocity = imu_config->gyro_stationary_threshold * imu_config->gyro_scale;
    *acceleration = imu_config->accel_stationary_threshold * imu_config->accel_scale;
    *heading_correction = heading_degrees_per_rotation;
}

/**
 * Gets the cached IMU angular velocity in deg/s, compensated for gyro bias.
 *
 * @param [out] values      The angular velocity vector.
 */
void pbio_imu_get_angular_velocity(pbio_geometry_xyz_t *values) {
    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &angular_velocity, values);
}

/**
 * Gets the cached IMU acceleration in mm/s^2.
 *
 * @param [out] values      The acceleration vector.
 */
void pbio_imu_get_acceleration(pbio_geometry_xyz_t *values) {
    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &acceleration, values);
}

/**
 * Gets the rotation along a particular axis of the robot frame.
 *
 * The resulting value makes sense only for one-dimensional rotations.
 *
 * @param [in]  axis        The axis to project the rotation onto.
 * @param [out] angle       The angle of rotation in degrees.
 * @return                  ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_ARG if axis has zero length.
 */
pbio_error_t pbio_imu_get_single_axis_rotation(pbio_geometry_xyz_t *axis, float *angle) {

    // Transform the single axis rotations to the robot frame.
    pbio_geometry_xyz_t rotation;
    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &single_axis_rotation, &rotation);

    // Get the requested scalar rotation along the given axis.
    return pbio_geometry_vector_project(axis, &rotation, angle);
}

/**
 * Gets which side of a hub points upwards.
 *
 * @return                  Which side is up.
 */
pbio_geometry_side_t pbio_imu_get_up_side(void) {
    // Up is which side of a unit box intersects the +Z vector first.
    // So read +Z vector of the inertial frame, in the body frame.
    // For now, this is the gravity vector. In the future, we can make this
    // slightly more accurate by using the full IMU orientation.
    return pbio_geometry_side_from_vector(&acceleration);
}

static float heading_offset = 0;

/**
 * Reads the estimated IMU heading in degrees, accounting for user offset and
 * user-specified heading correction scaling constant.
 *
 * Heading is defined as clockwise positive.
 *
 * @return                  Heading angle in the base frame.
 */
float pbio_imu_get_heading(void) {

    pbio_geometry_xyz_t heading_mapped;

    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &single_axis_rotation, &heading_mapped);

    return -heading_mapped.z * 360.0f / heading_degrees_per_rotation - heading_offset;
}

/**
 * Sets the IMU heading.
 *
 * This only adjusts the user offset without resetting anything in the
 * algorithm, so this can be called at any time.
 *
 * @param [in] desired_heading  The desired heading value.
 */
void pbio_imu_set_heading(float desired_heading) {
    heading_offset = pbio_imu_get_heading() + heading_offset - desired_heading;
}

/**
 * Gets the estimated IMU heading in control units through a given scale.
 *
 * This is mainly used to convert the heading to the right format for a
 * drivebase, which measures heading as the half the difference of the two
 * motor positions in millidegrees.
 *
 * Heading is defined as clockwise positive.
 *
 * @param [out]  heading               The heading angle in control units.
 * @param [out]  heading_rate          The heading rate in control units.
 * @param [in]   ctl_steps_per_degree  The number of control steps per heading degree.
 */
void pbio_imu_get_heading_scaled(pbio_angle_t *heading, int32_t *heading_rate, int32_t ctl_steps_per_degree) {

    // Heading in degrees of the robot.
    float heading_degrees = pbio_imu_get_heading();

    // Number of whole rotations in control units (in terms of wheels, not robot).
    heading->rotations = heading_degrees / (360000 / ctl_steps_per_degree);

    // The truncated part represents everything else.
    float truncated = heading_degrees - heading->rotations * (360000 / ctl_steps_per_degree);
    heading->millidegrees = truncated * ctl_steps_per_degree;

    // The heading rate can be obtained by a simple scale because it always fits.
    pbio_geometry_xyz_t angular_rate;
    pbio_imu_get_angular_velocity(&angular_rate);
    *heading_rate = (int32_t)(-angular_rate.z * ctl_steps_per_degree);
}

#endif // PBIO_CONFIG_IMU

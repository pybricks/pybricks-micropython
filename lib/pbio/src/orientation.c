// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdbool.h>
#include <string.h>

#include <pbdrv/imu.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/int_math.h>
#include <pbio/geometry.h>
#include <pbio/orientation.h>
#include <pbio/util.h>

#if PBIO_CONFIG_ORIENTATION_IMU

static pbdrv_imu_dev_t *imu_dev;
static pbdrv_imu_config_t *imu_config;

// This counter is a measure for calibration accuracy, roughly equivalent
// to the accumulative number of seconds it was stationary.
static uint32_t stationary_counter = 0;

// Cached sensor values that can be read at any time without polling again.
static pbio_geometry_xyz_t angular_velocity; // deg/s, already adjusted for bias.
static pbio_geometry_xyz_t acceleration;
static pbio_geometry_xyz_t gyro_bias;
static pbio_geometry_xyz_t heading;

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
        heading.values[i] += angular_velocity.values[i] * imu_config->sample_time;
    }
}

// Called by driver to process unfiltered gyro and accelerometer data recorded while stationary.
static void pbio_imu_handle_stationary_data_func(const int32_t *gyro_data_sum, const int32_t *accel_data_sum, uint32_t num_samples) {

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
void pbio_orientation_imu_init(void) {
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
pbio_error_t pbio_orientation_set_base_orientation(pbio_geometry_xyz_t *front_side_axis, pbio_geometry_xyz_t *top_side_axis) {

    pbio_error_t err = pbio_geometry_map_from_base_axes(front_side_axis, top_side_axis, &pbio_orientation_base_orientation);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    pbio_orientation_imu_set_heading(0.0f);

    return PBIO_SUCCESS;
}

/**
 * Checks if the IMU is currently stationary.
 *
 * @return    True if it has been stationary for about a second, false if moving.
 */
bool pbio_orientation_imu_is_stationary(void) {
    return pbdrv_imu_is_stationary(imu_dev);
}

/**
 * Sets the thresholds that define when the hub is stationary. When the
 * measurements are steadily below these levels, the orientation module
 * automatically recalibrates.
 *
 * @param [in]  angular_velocity Angular velocity threshold in deg/s.
 * @param [in]  acceleration     Acceleration threshold in mm/s^2
 */
void pbio_orientation_imu_set_stationary_thresholds(float angular_velocity, float acceleration) {
    int16_t gyro_threshold = pbio_int_math_bind(angular_velocity / imu_config->gyro_scale, 1, INT16_MAX);
    int16_t accl_threshold = pbio_int_math_bind(acceleration / imu_config->accel_scale, 1, INT16_MAX);
    pbdrv_imu_set_stationary_thresholds(imu_dev, gyro_threshold, accl_threshold);
}

/**
 * Gets the cached IMU angular velocity in deg/s, compensated for gyro bias.
 *
 * @param [out] values      The angular velocity vector.
 */
void pbio_orientation_imu_get_angular_velocity(pbio_geometry_xyz_t *values) {
    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &angular_velocity, values);
}

/**
 * Gets the cached IMU acceleration in mm/s^2.
 *
 * @param [out] values      The acceleration vector.
 */
void pbio_orientation_imu_get_acceleration(pbio_geometry_xyz_t *values) {
    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &acceleration, values);
}

/**
 * Gets which side of a hub points upwards.
 *
 * @return                  Which side is up.
 */
pbio_geometry_side_t pbio_orientation_imu_get_up_side(void) {
    // Up is which side of a unit box intersects the +Z vector first.
    // So read +Z vector of the inertial frame, in the body frame.
    // For now, this is the gravity vector. In the future, we can make this
    // slightly more accurate by using the full IMU orientation.
    return pbio_geometry_side_from_vector(&acceleration);
}

static float heading_offset = 0;

/**
 * Reads the estimated IMU heading in degrees, accounting for user offset.
 *
 * @return                  Heading angle in the base frame.
 */
float pbio_orientation_imu_get_heading(void) {

    pbio_geometry_xyz_t heading_mapped;

    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &heading, &heading_mapped);

    return heading_mapped.z - heading_offset;
}

/**
 * Sets the IMU heading.
 *
 * This only adjusts the user offset without resetting anything in the
 * algorithm, so this can be called at any time.
 *
 * @param [in] desired_heading  The desired heading value.
 */
void pbio_orientation_imu_set_heading(float desired_heading) {
    heading_offset = pbio_orientation_imu_get_heading() + heading_offset - desired_heading;
}

/**
 * Gets the estimated IMU heading in control units through a given scale.
 *
 * This is mainly used to convert the heading to the right format for a
 * drivebase, which measures heading as the half the difference of the two
 * motor positions in millidegrees.
 *
 * @param [out]  heading               The output angle object.
 * @param [in]   ctl_steps_per_degree  The number of control steps per heading degree.
 */
void pbio_orientation_imu_get_heading_scaled(pbio_angle_t *heading, int32_t ctl_steps_per_degree) {

    // Heading in degrees of the robot.
    float heading_degrees = pbio_orientation_imu_get_heading();

    // Number of whole rotations in control units (in terms of wheels, not robot).
    heading->rotations = heading_degrees / (360000 / ctl_steps_per_degree);

    // The truncated part represents everything else.
    float truncated = heading_degrees - heading->rotations * (360000 / ctl_steps_per_degree);
    heading->millidegrees = truncated * ctl_steps_per_degree;
}

#endif // PBIO_CONFIG_ORIENTATION_IMU

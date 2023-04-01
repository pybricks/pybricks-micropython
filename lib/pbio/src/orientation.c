// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <assert.h>

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

/**
 * Gets @p index and @p sign of the axis that passes through given @p side of
 * a box.
 * @param [in]  side    The requested side of the box.
 * @param [out] index   The index 0 (x), 1 (y), or 2 (z) of the axis.
 * @param [out] sign    The sign of the axis: 1 or -1.
 */
void pbio_orientation_side_get_axis(pbio_orientation_side_t side, uint8_t *index, int8_t *sign) {
    *index = side & 0x03;
    *sign = (side & 0x04) ? -1 : 1;
}

/**
 * Given index and sign of two base axes, finds the axis to complete a right
 * handed coordinate system.
 * @param [in] index   Array of three axis indexes. First and last are given,
 *                     middle axis index is computed.
 * @param [in] sign    Array of three axis signs. First and last are given,
 *                     middle axis sign is computed.
 */
void pbio_orientation_get_complementary_axis(uint8_t *index, int8_t *sign) {

    // Inputs must have valid axis index and sign.
    assert(index[0] < 3);
    assert(index[2] < 3);
    assert(sign[0] == 1 || sign[2] == 1);

    // Two axis cannot be parallel.
    assert(index[2] != index[0]);

    // The final axis is determined as the cross product v_1 = v_2 (x) v_0.
    // Since we know the axes are just base axis up to a sign, the result C
    // simply has the index that isn't used yet:
    index[1] = 3 - index[2] - index[0];

    // We still have so evaluate the cross product to get the sign. With inputs
    // j = {0, 2}, both input vectors v_0 and v_2 can be written in
    // terms of known information as:
    //
    //                 [ i_j == 0 ]
    //    v_j = s_j *  [ i_j == 1 ]
    //                 [ i_j == 2 ]
    //
    // where s_j is the sign of vector j, and i_j is the axis index of j.
    //
    // For example, if the first input has sign -1 and index 2, it is
    //
    //         [ 0]
    //   v_0 = [ 0]
    //         [-1]
    //
    // So it is the negative Z vector.
    //
    // Given the vectors v_0 and v_2, we evaluate the cross product formula:
    //
    // v_1 = + s_0 * s_2 * ((i_2==1)*(i_0==2)-(i_0==1)*(i_2==2)) * E_0
    //       - s_0 * s_2 * ((i_2==0)*(i_0==2)-(i_0==0)*(i_2==2)) * E_1
    //       + s_0 * s_2 * ((i_2==0)*(i_0==1)-(i_0==0)*(i_2==1)) * E_2
    //
    // But we already know which of these three vectors is nonzero, so we just
    // need to take the scalar value in front of it. This leaves us with one of
    // three results. Since the formulas are so similar, we can generalize it
    // to one result that depends on the known final axis index. This gives:
    sign[1] = sign[0] * sign[2] * (
        (index[0] == (index[1] + 2) % 3) * (index[2] == (index[1] + 1) % 3) -
        (index[0] == (index[1] + 1) % 3) * (index[2] == (index[1] + 2) % 3)
        );
}

/**
 * Gets the side of a unit-sized box through which a given vector passes first.
 *
 * @param [in]  vector  The input vector.
 * @return              The side through which the vector passes first.
 */
pbio_orientation_side_t pbio_orientation_side_from_vector(pbio_geometry_xyz_t *vector) {

    // Find index and sign of maximum component
    float abs_max = 0;
    uint8_t axis = 0;
    bool negative = true;
    for (uint8_t i = 0; i < 3; i++) {
        if (vector->values[i] > abs_max) {
            abs_max = vector->values[i];
            negative = false;
            axis = i;
        } else if (-vector->values[i] > abs_max) {
            abs_max = -vector->values[i];
            negative = true;
            axis = i;
        }
    }

    // Return as side enum value.
    return axis | (negative << 2);
}

#if PBIO_CONFIG_ORIENTATION_IMU

pbdrv_imu_dev_t *imu_dev;
pbdrv_imu_config_t *imu_config;

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
static void pbdrv_imu_handle_stationary_data_func(const int32_t *gyro_data_sum, const int32_t *accel_data_sum, uint32_t num_samples) {

    stationary_counter++;

    // The relative weight of the new data in order to build a long term
    // average of the data without maintaining a data buffer.
    float weight = stationary_counter >= 20 ? 0.05f : 1.0f / stationary_counter;

    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(gyro_bias.values); i++) {
        // Average gyro rate while stationary, indicating current bias.
        float average_now = gyro_data_sum[i] * imu_config->gyro_scale / num_samples;

        // Update bias at decreasing rate.
        gyro_bias.values[i] = gyro_bias.values[i] * (1 - weight) + weight * average_now;
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
    pbdrv_imu_set_data_handlers(imu_dev, pbio_imu_handle_frame_data_func, pbdrv_imu_handle_stationary_data_func);
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
 * Reads the current IMU angular velocity in deg/s, compensated for offset.
 *
 * @param [out] values      The angular velocity vector.
 */
void pbio_orientation_imu_get_angular_velocity(pbio_geometry_xyz_t *values) {
    pbio_geometry_vector_map(&pbio_orientation_base_orientation, &angular_velocity, values);
}

/**
 * Reads the current IMU acceleration in mm/s^2.
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
pbio_orientation_side_t pbio_orientation_imu_get_up_side(void) {
    // Up is which side of a unit box intersects the +Z vector first.
    // So read +Z vector of the inertial frame, in the body frame.
    // For now, this is the gravity vector. In the future, we can make this
    // slightly more accurate by using the full IMU orientation.
    return pbio_orientation_side_from_vector(&acceleration);
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
 * Resets the IMU heading.
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

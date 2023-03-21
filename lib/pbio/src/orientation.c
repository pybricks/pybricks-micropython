// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <assert.h>

#include <stdbool.h>
#include <string.h>

#include <pbdrv/imu.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/orientation.h>

#if PBIO_CONFIG_ORIENTATION

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

#if PBIO_CONFIG_ORIENTATION_IMU

pbdrv_imu_dev_t *imu_dev;
pbdrv_imu_config_t *imu_config;

// This counter is a measure for calibration accuracy, roughly equivalent
// to the accumulative number of seconds it was stationary.
static uint32_t stationary_counter = 0;

static float angular_velocity[3];
static float acceleration[3];
static float gyro_bias[3];

static float heading_rate_last = 0;
static float heading = 0;
static float heading_offset = 0;

void pbio_imu_handle_frame_data_func(int16_t *data) {
    for (uint8_t i = 0; i < 3; i++) {
        angular_velocity[i] = data[i] * imu_config->gyro_scale - gyro_bias[i];
        acceleration[i] = data[i + 3] * imu_config->accel_scale;
    }
    heading += (heading_rate_last + angular_velocity[2]) * imu_config->sample_time / 2;
    heading_rate_last = angular_velocity[2];
}

void pbdrv_imu_handle_stationary_data_func(int32_t *gyro_data_sum, int32_t *accel_data_sum, uint32_t num_samples) {

    stationary_counter++;

    // The relative weight of the new data in order to build a long term
    // average of the data without maintaining a data buffer.
    float weight = stationary_counter >= 20 ? 0.05f : 1.0f / stationary_counter;

    for (uint8_t i = 0; i < 3; i++) {
        // Average gyro rate while stationary, indicating current bias.
        float average_now = gyro_data_sum[i] * imu_config->gyro_scale / num_samples;

        // Update bias at decreasing rate.
        gyro_bias[i] = gyro_bias[i] * (1 - weight) + weight * average_now;
    }
}

void pbio_orientation_imu_init(void) {
    pbio_error_t err = pbdrv_imu_get_imu(&imu_dev, &imu_config);
    if (err != PBIO_SUCCESS) {
        return;
    }
    pbdrv_imu_set_data_handlers(imu_dev, pbio_imu_handle_frame_data_func, pbdrv_imu_handle_stationary_data_func);
}

/**
 * Gets a counter value indicating how long the IMU was stationary.
 *
 * @return    How many subsequent samples were stationary.
 */
uint32_t pbio_orientation_imu_get_stationary_count(void) {
    return stationary_counter;
}

/**
 * Reads the current IMU angular velocity in deg/s, compensated for offset.
 * @param [in] imu_dev      The driver instance.
 * @param [out] values      An array of 3 32-bit float values to hold the result.
 */
void pbio_orientation_imu_get_angular_velocity(float *values) {
    memcpy(values, angular_velocity, sizeof(angular_velocity));
}

/**
 * Reads the current IMU angular velocity in deg/s, compensated for offset.
 * @param [in] imu_dev      The driver instance.
 * @param [out] values      An array of 3 32-bit float values to hold the result.
 */
void pbio_orientation_imu_get_acceleration(float *values) {
    memcpy(values, acceleration, sizeof(acceleration));
}

/**
 * Reads the estimated IMU heading in degrees.
 * @param [in] imu_dev      The driver instance.
 * @return                  Heading angle.
 */
float pbio_orientation_imu_get_heading(void) {
    return heading - heading_offset;
}

/**
 * Sets the IMU heading in degrees.
 * @param [in] imu_dev      The driver instance.
 * @return                  Heading angle.
 */
void pbio_orientation_imu_set_heading(float desired_heading) {
    heading_offset = heading - desired_heading;
}

#endif // PBIO_CONFIG_ORIENTATION_IMU

#endif // PBIO_CONFIG_ORIENTATION

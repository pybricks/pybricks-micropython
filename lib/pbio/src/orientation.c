// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <assert.h>

#include <stdbool.h>

#include <pbdrv/imu.h>

#include <pbio/config.h>
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

/**
 * Coordinate type with x, y, and z floating point values.
 */
typedef struct _pbio_orientation_xyz_t {
    float x; /**< X coordinate.*/
    float y; /**< Y coordinate.*/
    float z; /**< Z coordinate.*/
} pbio_orientation_xyz_t;

// Revisit: Create pbio API for IMUs. For now assume there is only one global IMU.
static pbio_orientation_xyz_t average_gyro_data;
static uint32_t stationary_counter = 0;

/**
 * Update gyro offset with new stationary data gathered by the driver. Expected to
 * be called approximately once per second of stationary data.
 *
 * @param [in] short_term_average_gyro_data  Average x, y, and z gyro rate values over the past second.
 */
void pbio_orientation_imu_update_gyro_rate_bias(float *short_term_average_gyro_data) {

    // This counter is a measure for calibration accuracy, roughly equivalent
    // to the accumulative number of seconds it was stationary.
    stationary_counter++;

    // The relative weight of the new data in order to build a long term
    // average of the data without maintaining a data buffer.
    float weight = stationary_counter >= 20 ? 0.05f : 1.0f / stationary_counter;
    average_gyro_data.x = average_gyro_data.x * (1 - weight) + weight * short_term_average_gyro_data[0];
    average_gyro_data.y = average_gyro_data.y * (1 - weight) + weight * short_term_average_gyro_data[1];
    average_gyro_data.z = average_gyro_data.z * (1 - weight) + weight * short_term_average_gyro_data[2];
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
void pbio_orientation_imu_get_angular_velocity(pbdrv_imu_dev_t *imu_dev, float *values) {

    pbio_orientation_xyz_t gyro_data = {0};
    pbdrv_imu_gyro_read(imu_dev, (float *)&gyro_data);
    values[0] = gyro_data.x - average_gyro_data.x;
    values[1] = gyro_data.y - average_gyro_data.y;
    values[2] = gyro_data.z - average_gyro_data.z;
}

float yaw_rate_last;
float heading;
float heading_offset = 0;

/**
 * Callback that runs when IMU driver has new data.
 * @param [in] imu_dev     The driver instance.
 */
void pbio_orientation_imu_new_data_handler(pbdrv_imu_dev_t *imu_dev) {
    float gyro_rate[3];
    pbio_orientation_imu_get_angular_velocity(imu_dev, gyro_rate);

    // REVISIT: This should be 2 x 833, but it is slightly off. Need to
    // review actual sample rate.
    heading += (yaw_rate_last + gyro_rate[2]) / (1639);
    yaw_rate_last = gyro_rate[2];
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
void pbio_orientation_imu_set_heading(pbdrv_imu_dev_t *imu_dev, float desired_heading) {
    heading_offset = heading - desired_heading;
}

#endif // PBIO_CONFIG_ORIENTATION

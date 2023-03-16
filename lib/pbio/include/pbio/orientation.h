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

#if PBIO_CONFIG_ORIENTATION

void pbio_orientation_side_get_axis(pbio_orientation_side_t side, uint8_t *index, int8_t *sign);

void pbio_orientation_get_complementary_axis(uint8_t *index, int8_t *sign);

#if PBIO_CONFIG_ORIENTATION_IMU

void pbio_orientation_imu_new_data_handler(pbdrv_imu_dev_t *imu_dev);

void pbio_orientation_imu_update_gyro_rate_bias(float *average_gyro_data);

uint32_t pbio_orientation_imu_get_stationary_count(void);

void pbio_orientation_imu_get_angular_velocity(pbdrv_imu_dev_t *imu_dev, float *values);

float pbio_orientation_imu_get_heading(pbdrv_imu_dev_t *imu_dev);

void pbio_orientation_imu_set_heading(pbdrv_imu_dev_t *imu_dev, float desired_heading);

#endif // #if PBIO_CONFIG_ORIENTATION_IMU

#else // PBIO_CONFIG_ORIENTATION

static inline void pbio_orientation_side_get_axis(pbio_orientation_side_t side, uint8_t *index, int8_t *sign) {
}

static inline void pbio_orientation_get_complementary_axis(uint8_t *index, int8_t *sign) {
}

#if PBIO_CONFIG_ORIENTATION_IMU

static inline void pbio_orientation_imu_get_angular_velocity(pbdrv_imu_dev_t *imu_dev, float *values) {
}

static inline uint32_t pbio_orientation_imu_get_stationary_count(void) {
    return 0;
}

static inline void pbio_orientation_imu_update_gyro_rate_bias(float *average_gyro_data) {
}

static inline float pbio_orientation_imu_get_heading(pbdrv_imu_dev_t *imu_dev) {
    return 0.0f;
}

static inline void pbio_orientation_imu_set_heading(pbdrv_imu_dev_t *imu_dev, float desired_heading) {
}

#endif // #if PBIO_CONFIG_ORIENTATION_IMU

#endif // PBIO_CONFIG_ORIENTATION

#endif // _PBIO_ORIENTATION_H_

/** @} */

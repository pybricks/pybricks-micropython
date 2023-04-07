// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

/**
 * @addtogroup IMUDriver Driver: Inertial Measurement Unit (IMU)
 * @{
 */

#ifndef PBDRV_IMU_H
#define PBDRV_IMU_H

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * Opaque handle to an IMU device instance.
 */
typedef struct _pbdrv_imu_dev_t pbdrv_imu_dev_t;

/**
 * IMU configuration used to convert raw data to physical units.
 */
typedef struct {
    /** The average time in seconds between samples. */
    float sample_time;
    /** Angular velocity in deg/s for every unit of raw gyro data. */
    float gyro_scale;
    /** Acceleration in mm/s^2 for every unit of raw accelerometer data. */
    float accel_scale;
    /** Angular velocity threshold below which the IMU is considered stationary, in raw units. */
    int16_t gyro_stationary_threshold;
    /** Acceleration threshold below which the IMU is considered stationary, in raw units. */
    int16_t accel_stationary_threshold;
} pbdrv_imu_config_t;

#if PBDRV_CONFIG_IMU

/**
 * Gets the one and only IMU device instance.
 *
 * @param [out] imu_dev     The device instance.
 * @param [out] config      IMU configuration.
 * @returns                 ::PBIO_ERROR_AGAIN if the driver has not been
 *                          initalized, ::PBIO_ERROR_NOT_SUPPORTED if the
 *                          driver is not enabled or ::PBIO_SUCCESS on success.
 */
pbio_error_t pbdrv_imu_get_imu(pbdrv_imu_dev_t **imu_dev, pbdrv_imu_config_t **config);

/**
 * Tests if imu has been stationary for about one second.
 *
 * @param [in]  imu_dev     The IMU device instance.
 * @return                  @c true if stationary, @c false if not.
 */
bool pbdrv_imu_is_stationary(pbdrv_imu_dev_t *imu_dev);

/**
 * Callback to process one frame of unfiltered gyro and accelerometer data.
 *
 * @param [in]  data        Array with unscaled gyro (xyz) and acceleration (xyz) samples to process.
 */
typedef void (*pbdrv_imu_handle_frame_data_func_t)(int16_t *data);

/**
 * Callback to process @p num_samples unfiltered gyro and accelerometer data
 * samples recorded while the sensor was stationary.
 *
 * @param [in]  gyro_data_sum   Array with the sums of stationary gyro samples.
 * @param [in]  accel_data_sum  Array with the sums of stationary accelerometer samples.
 * @param [in]  num_samples     Number of samples summed.
 */
typedef void (*pbdrv_imu_handle_stationary_data_func_t)(const int32_t *gyro_data_sum, const int32_t *accel_data_sum, uint32_t num_samples);

/**
 * Sets the data handlers for processing new data.
 *
 * @param [in]  imu_dev                The IMU device instance.
 * @param [in]  frame_data_func        Callback that handles one data frame.
 * @param [in]  stationary_data_func   Callback that handles multiple stationary data frames.
 */
void pbdrv_imu_set_data_handlers(pbdrv_imu_dev_t *imu_dev, pbdrv_imu_handle_frame_data_func_t frame_data_func, pbdrv_imu_handle_stationary_data_func_t stationary_data_func);

#else // PBDRV_CONFIG_IMU

static inline pbio_error_t pbdrv_imu_get_imu(pbdrv_imu_dev_t **imu_dev, pbdrv_imu_config_t **config) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline bool pbdrv_imu_is_stationary(pbdrv_imu_dev_t *imu_dev) {
    return false;
}

#endif // PBDRV_CONFIG_IMU

#endif // PBDRV_IMU_H

/** @} */

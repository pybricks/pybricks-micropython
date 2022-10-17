// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

/**
 * @addtogroup IMUDriver Driver: Inertial Measurement Unit (IMU)
 * @{
 */

#ifndef PBDRV_IMU_H
#define PBDRV_IMU_H

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * Opaque handle to an IMU device instance.
 */
typedef struct _pbdrv_imu_dev_t pbdrv_imu_dev_t;

#if PBDRV_CONFIG_IMU

/**
 * Gets the one and only IMU device instance.
 * @param [out] imu_dev     The device instance.
 * @returns                 ::PBIO_ERROR_AGAIN if the driver has not been
 *                          initalized, ::PBIO_ERROR_NOT_SUPPORTED if the
 *                          driver is not enabled or ::PBIO_SUCCESS on success.
 */
pbio_error_t pbdrv_imu_get_imu(pbdrv_imu_dev_t **imu_dev);

/**
 * Reads the current IMU acceleration in m/s^2.
 * @param [in]  imu_dev     The IMU device instance.
 * @param [out] values      An array of 3 32-bit float values to hold the result.
 */
void pbdrv_imu_accel_read(pbdrv_imu_dev_t *imu_dev, float *values);

/**
 * Reads the current IMU gyro rate in deg/s.
 * @param [in]  imu_dev     The IMU device instance.
 * @param [out] values      An array of 3 32-bit float values to hold the result.
 */
void pbdrv_imu_gyro_read(pbdrv_imu_dev_t *imu_dev, float *values);

/**
 * Reads the current IMU temperature in deg C.
 * @param [in]  imu_dev     The IMU device instance.
 * @returns                 The temperature value.
 */
float pbdrv_imu_temperature_read(pbdrv_imu_dev_t *imu_dev);

#else // PBDRV_CONFIG_IMU

static inline pbio_error_t pbdrv_imu_get_imu(pbdrv_imu_dev_t **imu_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_imu_accel_read(pbdrv_imu_dev_t *imu_dev, float *values) {
}

static inline void pbdrv_imu_gyro_read(pbdrv_imu_dev_t *imu_dev, float *values) {
}

static inline float pbdrv_imu_temperature_read(pbdrv_imu_dev_t *imu_dev) {
    return 0;
}

#endif // PBDRV_CONFIG_IMU

#endif // PBDRV_IMU_H

/** @} */

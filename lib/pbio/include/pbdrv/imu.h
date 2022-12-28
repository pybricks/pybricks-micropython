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
 * Reads the current IMU estimate quaternion in deg/s.
 * @param [in]  imu_dev     The IMU device instance.
 * @param [out] values      An array of 4 32-bit float values to hold the result.
 */
void pbdrv_imu_quaternion_read(pbdrv_imu_dev_t *imu_dev, float *values);

/**
 * resets the current IMU estimate.
 * @param [in]  imu_dev     The IMU device instance.
 */
void pbdrv_imu_reset_heading(pbdrv_imu_dev_t *imu_dev);

/**
 * resets the current IMU estimate.
 * @param [in]  imu_dev     The IMU device instance.
 * @param [in]  Kp          Proportional Gain
 * @param [in]  Ki          Integral Gain
 */
void pbdrv_imu_set_mahony_gains(pbdrv_imu_dev_t *imu_dev,float Kp,float Ki);

/**
 * Starts gyro calibration.
 * @param [in]  imu_dev     The IMU device instance.
 */
void pbdrv_imu_start_gyro_calibration(pbdrv_imu_dev_t *imu_dev);

/**
 * Ends gyro calibration, stores offsets, and returns gyro bias.
 * @param [in]  imu_dev     The IMU device instance.
 * @param [out] values      An array of 3 32-bit float values to hold the gyro bias.
 */
void pbdrv_imu_stop_gyro_calibration(pbdrv_imu_dev_t *imu_dev, float *values);
/**
 * Sets gyro bias offsets.
 * @param [in]  imu_dev     The IMU device instance.
 * @param [in]  X           X bias in deg/s.
 * @param [in]  Y           Y bias in deg/s.
 * @param [in]  Z           Z bias in deg/s.
 */
void pbdrv_imu_set_gyro_bias(pbdrv_imu_dev_t* imu_dev,float X,float Y,float Z);
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

static inline void pbdrv_imu_quaternion_read(pbdrv_imu_dev_t *imu_dev, float *values) {
}

static inline void pbdrv_imu_reset_heading(pbdrv_imu_dev_t *imu_dev) {
}

static inline void pbdrv_imu_stop_gyro_calibration(pbdrv_imu_dev_t *imu_dev, float *values) {
}

static inline void pbdrv_imu_start_gyro_calibration(pbdrv_imu_dev_t *imu_dev) {
}

static inline void pbdrv_imu_set_gyro_bias(pbdrv_imu_dev_t* imu_dev,float X,float Y,float Z) {
}

static inline void pbdrv_imu_set_mahony_gains(pbdrv_imu_dev_t *imu_dev,float Kp,float Ki){
}

static inline float pbdrv_imu_temperature_read(pbdrv_imu_dev_t *imu_dev) {
    return 0;
}

#endif // PBDRV_CONFIG_IMU

#endif // PBDRV_IMU_H

/** @} */

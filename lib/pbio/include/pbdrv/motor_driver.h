// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020,2022 The Pybricks Authors

/**
 * @addtogroup MotorDriverDriver Driver: Motor driver chip
 * @{
 */

#ifndef _PBDRV_MOTOR_DRIVER_H_
#define _PBDRV_MOTOR_DRIVER_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * The duty cycle value cooresponding to 100% duty cycle.
 */
#define PBDRV_MOTOR_DRIVER_MAX_DUTY 1000

/**
 * Handle to a motor driver instance.
 */
typedef struct _pbdrv_motor_driver_dev_t pbdrv_motor_driver_dev_t;

#if PBDRV_CONFIG_MOTOR_DRIVER

#ifndef PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV
#error "missing PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV config option"
#endif

/**
 * Gets a motor driver instance handle.
 *
 * @param [in]  id      The platform device instance identifier.
 * @param [out] driver  The motor driver instance handle.
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_NO_DEV if @p id is out of range
 *                      ::PBIO_ERROR_AGAIN if the driver is not intialized yet.
 */
pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver);

/**
 * Instructs the motor to coast freely.
 * @param [in]  driver  The motor driver instance.
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver);

/**
 * Sets the PWM duty cycle for the motor. Setting a duty cycle of 0 will "brake" the motor.
 *
 * Passing a @p duty_cycle value outside of the allowed range will result in
 * undefined behavior. Use `pbio_math_clamp(duty, PBDRV_MOTOR_DRIVER_MAX_DUTY)`
 * if the value may be out of range.
 *
 * @param [in]  driver      The motor driver instance.
 * @param [in]  duty_cycle  The duty cycle -::PBDRV_MOTOR_DRIVER_MAX_DUTY to ::PBDRV_MOTOR_DRIVER_MAX_DUTY
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle);

#else

static inline pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif // _PBDRV_MOTOR_DRIVER_H_

/** @} */

// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

/**
 * @addtogroup DCMotor pbio/dcmotor: DC Motor
 *
 * Interface for basic interaction with an encoderless brushed DC Motor.
 * @{
 */

#ifndef _PBIO_DCMOTOR_H_
#define _PBIO_DCMOTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/parent.h>
#include <pbio/port.h>

/**
 * Direction a motor turns in when a positive voltage is applied.
 */
typedef enum {
    PBIO_DIRECTION_CLOCKWISE,         /**< Positive means clockwise. */
    PBIO_DIRECTION_COUNTERCLOCKWISE,  /**< Positive means counterclockwise. */
} pbio_direction_t;

/**
 * Actuation types that can be applied by a dc motor.
 */
typedef enum {
    PBIO_DCMOTOR_ACTUATION_COAST,   /**< Coast the motor. */
    PBIO_DCMOTOR_ACTUATION_BRAKE,   /**< Brake the motor. */
    PBIO_DCMOTOR_ACTUATION_VOLTAGE, /**< Apply a voltage. */
    PBIO_DCMOTOR_ACTUATION_TORQUE,  /**< Apply a torque. */
} pbio_dcmotor_actuation_t;

/**
 * DC Motor instance.
 */
typedef struct _pbio_dcmotor_t {
    /** Port to which this motor is attached. */
    pbio_port_id_t port;
    /** Direction for positive speeds. */
    pbio_direction_t direction;
    /** Currently active actuation type. */
    pbio_dcmotor_actuation_t actuation_now;
    /** Currently applied voltage. */
    int32_t voltage_now;
    /** Maximum allowed voltage. */
    int32_t max_voltage;
    /** Parent object (like a servo) that uses this motor. */
    pbio_parent_t parent;
    /** Low level motor driver. */
    pbdrv_motor_driver_dev_t *motor_driver;
} pbio_dcmotor_t;

#if PBIO_CONFIG_DCMOTOR

/** @cond INTERNAL */
void pbio_dcmotor_stop_all(bool clear_parents);
pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor);
pbio_error_t pbio_dcmotor_set_voltage(pbio_dcmotor_t *dcmotor, int32_t voltage);
int32_t pbio_dcmotor_get_max_voltage(pbio_iodev_type_id_t id);
/** @endcond */

/** @name Initialization Functions */
/**@{*/
pbio_error_t pbio_dcmotor_get_dcmotor(pbio_port_id_t port, pbio_dcmotor_t **dcmotor);
pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, pbio_direction_t direction);
pbio_error_t pbio_dcmotor_close(pbio_dcmotor_t *dcmotor);
/**@}*/

/** @name Status Functions */
/**@{*/
void pbio_dcmotor_get_state(const pbio_dcmotor_t *dcmotor, pbio_dcmotor_actuation_t *actuation, int32_t *voltage_now);
void pbio_dcmotor_get_settings(const pbio_dcmotor_t *dcmotor, int32_t *max_voltage);
/**@}*/

/** @name Operation Functions */
/**@{*/
pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t max_voltage);
pbio_error_t pbio_dcmotor_user_command(pbio_dcmotor_t *dcmotor, bool coast, int32_t voltage);
/**@}*/

#else

static inline void pbio_dcmotor_stop_all(bool clear_parents) {
}

static inline pbio_error_t pbio_dcmotor_get_dcmotor(pbio_port_id_t port, pbio_dcmotor_t **dcmotor) {
    *dcmotor = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, pbio_direction_t direction) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_dcmotor_get_state(const pbio_dcmotor_t *dcmotor, pbio_dcmotor_actuation_t *actuation, int32_t *voltage_now) {
    *actuation = PBIO_DCMOTOR_ACTUATION_COAST;
    *voltage_now = 0;
}

static inline int32_t pbio_dcmotor_get_max_voltage(pbio_iodev_type_id_t id) {
    return 0;
}

static inline void pbio_dcmotor_get_settings(const pbio_dcmotor_t *dcmotor, int32_t *max_voltage) {
    *max_voltage = 0;
}

static inline pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t max_voltage) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_dcmotor_set_voltage(pbio_dcmotor_t *dcmotor, int32_t voltage) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_dcmotor_user_command(pbio_dcmotor_t *dcmotor, bool coast, int32_t voltage) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_DCMOTOR

#endif // _PBIO_DCMOTOR_H_

/** @} */

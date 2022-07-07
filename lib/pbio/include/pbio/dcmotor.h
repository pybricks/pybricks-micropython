// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup DCMotor pbio: DC Motor
 *
 * Interface for basic interaction with an encoderless brushed DC Motor.
 * @{
 */

#ifndef _PBIO_DCMOTOR_H_
#define _PBIO_DCMOTOR_H_

#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/config.h>
#include <pbio/iodev.h>
#include <pbio/parent.h>
#include <pbio/port.h>

typedef enum {
    PBIO_DIRECTION_CLOCKWISE,         /**< Positive means clockwise */
    PBIO_DIRECTION_COUNTERCLOCKWISE,  /**< Positive means counterclockwise */
} pbio_direction_t;

typedef enum {
    PBIO_DCMOTOR_ACTUATION_COAST,   /**< Coast the motor */
    PBIO_DCMOTOR_ACTUATION_BRAKE,   /**< Brake the motor */
    PBIO_DCMOTOR_ACTUATION_VOLTAGE, /**< Apply a voltage */
    PBIO_DCMOTOR_ACTUATION_TORQUE,  /**< Apply a torque */
} pbio_dcmotor_actuation_t;

typedef struct _pbio_dcmotor_t {
    pbio_port_id_t port;                    /**< Port to which this motor is attached */
    pbio_direction_t direction;             /**< Direction for positive speeds. */
    pbio_dcmotor_actuation_t actuation_now; /**< Currently active actuation type. */
    int32_t voltage_now;                    /**< Currently applied voltage. */
    int32_t max_voltage;                    /**< Maximum allowed voltage. */
    pbio_parent_t parent;                   /**< Parent object (like a servo) that uses this motor. */
    pbdrv_motor_driver_dev_t *motor_driver; /**< Low level motor driver */
} pbio_dcmotor_t;

#if PBIO_CONFIG_DCMOTOR

// Global reset
void pbio_dcmotor_stop_all(bool clear_parents);

// Setup and status
pbio_error_t pbio_dcmotor_get_dcmotor(pbio_port_id_t port, pbio_dcmotor_t **dcmotor);
pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, pbio_direction_t direction);
void pbio_dcmotor_get_state(pbio_dcmotor_t *dcmotor, pbio_dcmotor_actuation_t *actuation, int32_t *voltage_now);

// Settings
int32_t pbio_dcmotor_get_max_voltage(pbio_iodev_type_id_t id);
pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t max_voltage);
void pbio_dcmotor_get_settings(pbio_dcmotor_t *dcmotor, int32_t *max_voltage);

// Actuation for system purposes
pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor);
pbio_error_t pbio_dcmotor_set_voltage(pbio_dcmotor_t *dcmotor, int32_t voltage);

// Actuation for end users
pbio_error_t pbio_dcmotor_user_command(pbio_dcmotor_t *dcmotor, bool coast, int32_t voltage);

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

static inline void pbio_dcmotor_get_state(pbio_dcmotor_t *dcmotor, pbio_dcmotor_actuation_t *actuation, int32_t *voltage_now) {
    *actuation = PBIO_DCMOTOR_ACTUATION_COAST;
    *voltage_now = 0;
}

static inline int32_t pbio_dcmotor_get_max_voltage(pbio_iodev_type_id_t id) {
    return 0;
}

static inline void pbio_dcmotor_get_settings(pbio_dcmotor_t *dcmotor, int32_t *max_voltage) {
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

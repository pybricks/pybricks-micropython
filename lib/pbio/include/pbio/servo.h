// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

/**
 * @addtogroup Servo pbio/servo: Servo control functions
 *
 * API for motors with position feedback.
 * @{
 */

#ifndef _PBIO_SERVO_H_
#define _PBIO_SERVO_H_

#include <pbio/config.h>

#if PBIO_CONFIG_SERVO

#if PBIO_CONFIG_SERVO_NUM_DEV != PBIO_CONFIG_DCMOTOR_NUM_DEV
#error "Number of DC Motors expected to be equal to number of Servo Motors."
#endif

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/counter.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/dcmotor.h>
#include <pbio/tacho.h>
#include <pbio/trajectory.h>
#include <pbio/control.h>
#include <pbio/observer.h>
#include <pbio/logger.h>

#include <pbio/iodev.h>

/** Number of values per row when servo data logger is active. */
#define PBIO_SERVO_LOGGER_NUM_COLS (10)

/**
 * The servo system combines a dcmotor and rotation sensor with a controller
 * to provide speed and position control.
 *
 * All public pbio servo calls (pbio_servo_...) work with angles expressed
 * in degrees and speeds in degrees per second, measured at the output
 * of the external gear train, unless stated otherwise.
 *
 * Internally, the servo controller operates using millidegrees, measured at
 * the motor shaft. Scaling happens through the gear ratio value given during
 * the servo setup.
 */
typedef struct _pbio_servo_t {
    /**
     * The dcmotor being controlled.
     */
    pbio_dcmotor_t *dcmotor;
    /**
     * The tacho device that measures the motor angle.
     */
    pbio_tacho_t *tacho;
    /**
     * The controller for this servo.
     */
    pbio_control_t control;
    /**
     * Luenberger state observer to estimate motor speed.
     */
    pbio_observer_t observer;
    /**
     * Structure with data log settings and pointer to data buffer if active.
     */
    pbio_log_t log;
    /**
     * Link to parent object that uses this servo, like a drive base.
     */
    pbio_parent_t parent;
    /**
     * Internal flag used to set whether the servo state update loop should
     * keep running. This is false when the servo is unplugged or other errors
     * occur.
     */
    bool run_update_loop;
} pbio_servo_t;

/**
 * A minimal set of constant parameters for each motor type. All other
 * defaults are derived at runtime.
 *
 * This is a somewhat arbitrary combination of settings from:
 * - pbio/control_settings
 * - pbio/observer
 *
 * Any setting that does not have the same default across all motor types can
 * be added here, with the appropriate conversion made in servo_settings to
 * set it in the actual mutable settings structure.
 *
 * This mainly saves space compared to having fully populated structures for
 * each motor. It also allows us to ensure a "correct" default configuration,
 * without settings that conflict each other.
 */
typedef struct _pbio_servo_settings_reduced_t {
    /**
     * Type identifier indicating which motor it is.
     */
    pbio_iodev_type_id_t id;
    /**
     * Physical model parameter for this type of motor
     */
    const pbio_observer_model_t *model;
    /**
     * The rated maximum speed (deg/s), approximately equivalent to "100%" speed in other apps.
     */
    int32_t rated_max_speed;
    /**
     * Precision profile. This is a measure for "precision" (low values)
     * versus "smoothness" (high value), expressed roughly as the allowed
     * deviation (in degrees) from a target angle to be considered on target.
     * Several other settings such as kp and kd are derived from this value
     * such that the controller can still always reach the given target.
     */
    int32_t precision_profile;
    /**
     * Feedback gain (mV/deg) to correct the observer for low estimation errors.
     */
    int32_t feedback_gain_low;
    /**
     * Threshold speed below which to use a lower kp constant.
     */
    int32_t pid_kp_low_speed_threshold;
} pbio_servo_settings_reduced_t;

/** @name Initialization Functions */
/**@{*/
pbio_error_t pbio_servo_get_servo(pbio_port_id_t port, pbio_servo_t **srv);
pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, int32_t gear_ratio, bool reset_angle, int32_t precision_profile);
/**@}*/

/** @cond INTERNAL */
pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_dcmotor_actuation_t actuation_type, int32_t payload);
const pbio_servo_settings_reduced_t *pbio_servo_get_reduced_settings(pbio_iodev_type_id_t id);
void pbio_servo_update_all(void);
/** @endcond */

/** @name Status Functions */
/**@{*/
pbio_error_t pbio_servo_get_state_control(pbio_servo_t *srv, pbio_control_state_t *state);
pbio_error_t pbio_servo_get_state_user(pbio_servo_t *srv, int32_t *angle, int32_t *speed);
pbio_error_t pbio_servo_get_speed_user(pbio_servo_t *srv, uint32_t window, int32_t *speed);
bool pbio_servo_update_loop_is_running(pbio_servo_t *srv);
pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled, uint32_t *stall_duration);
pbio_error_t pbio_servo_get_load(pbio_servo_t *srv, int32_t *load);
/**@}*/

/** @name Operation Functions */
/**@{*/
pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs);
pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed);
pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, uint32_t duration, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target);
/**@}*/

#endif // PBIO_CONFIG_SERVO

#endif // _PBIO_SERVO_H_

/** @} */

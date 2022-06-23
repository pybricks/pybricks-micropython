// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup Servo Motor: Servo control functions.
 *
 * API for motors with position feedback (servos).
 * @{
 */

#ifndef _PBIO_SERVO_H_
#define _PBIO_SERVO_H_

#include <pbio/config.h>

#if PBIO_CONFIG_SERVO

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

#define PBIO_SERVO_LOG_COLS (9)

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
 * Gets pointer to static servo instance using port id.
 *
 * @param [in]  port        Port identifier.
 * @param [out] srv         Pointer to servo object.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_get_servo(pbio_port_id_t port, pbio_servo_t **srv);

/**
 * Sets up the servo instance to be used in an application.
 *
 * @param [in]  srv         The servo instance.
 * @param [in]  direction   The direction of positive rotation.
 * @param [in]  gear_ratio  The ratio between motor rotation (millidegrees) and the gear train output (degrees).
 * @param [in]  reset_angle If true, reset the current angle to the current absolute position if supported or 0.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, int32_t gear_ratio, bool reset_angle);

/**
 * Gets the state of the servo update loop.
 *
 * This becomes true after a successful call to pbio_servo_setup and becomes
 * false when there is an error. Such as when the cable is unplugged.
 *
 * @param [in]  srv         The servo instance
 * @return                  True if up and running, false if not.
 */
bool pbio_servo_update_loop_is_running(pbio_servo_t *srv);

/**
 * Loads device specific model parameters and control settings.
 *
 * @param [out]  control_settings   Control settings like PID constants.
 * @param [out]  model              Model parameters for the state observer.
 * @param [in]   id                 Type identifier for which to look up the settings.
 * @return                          Error code.
 */
pbio_error_t pbio_servo_load_settings(pbio_control_settings_t *control_settings, const pbio_observer_model_t **model, pbio_iodev_type_id_t id);


/**
 * Gets the servo state in units of control.
 *
 * @param [in]  srv         The servo instance.
 * @param [out] state       The system state object in units of control.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_get_state_control(pbio_servo_t *srv, pbio_control_state_t *state);

/**
 * Gets the servo state in units of degrees at the output.
 *
 * @param [in]  srv         The servo instance.
 * @param [out] angle       Angle in degrees.
 * @param [out] speed       Angular velocity in degrees per second.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_get_state_user(pbio_servo_t *srv, int32_t *angle, int32_t *speed);

/**
 * Resets the servo angle to a given value.
 *
 * @param [in]  srv          The servo instance.
 * @param [in]  reset_angle  Angle that servo should now report in degrees.
 * @param [in]  reset_to_abs If true, ignores reset_angle and resets to absolute angle marked on shaft instead.
 * @return                   Error code.
 */
pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs);

/**
 * Stops ongoing controlled motion to coast, brake, or hold the servo.
 *
 * @param [in]  srv           The servo instance.
 * @param [in]  on_completion Coast, brake, or hold after stopping the controller.
 * @return                    Error code.
 */
pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_control_on_completion_t on_completion);

/**
 * Stops ongoing controlled motion and applies a given voltage instead.
 *
 * The voltage is applied by applying a duty cycle that is scaled by the
 * average battery voltage at this time. The voltage is not subsequently
 * adjusted as the battery discharges.
 *
 * @param [in]  srv           The servo instance.
 * @param [in]  voltage       Voltage to apply in mV.
 * @return                    Error code.
 */
pbio_error_t pbio_servo_set_voltage_passive(pbio_servo_t *srv, int32_t voltage);

/**
 * Checks whether servo is stalled. If the servo is actively controlled,
 * it is stalled when the controller cannot maintain the target speed or
 * position while using maximum allowed torque. If control is not active,
 * it uses the observer to estimate whether it is stalled.
 *
 * @param [in]  srv             The servo instance.
 * @param [out] stalled         True if servo is stalled, false if not.
 * @param [out] stall_duration  For how long it has been stalled (ms).
 * @return                      Error code.
 */
pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled, uint32_t *stall_duration);

/**
 * Starts running the servo at a given speed.
 *
 * @param [in]  srv             The servo instance.
 * @param [in]  speed           Angular velocity in degrees per second.
 * @return                      Error code.
 */
pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed);

/**
 * Runs the servo at a given speed and stops after a given duration.
 *
 * @param [in]  srv            The control instance.
 * @param [in]  speed          Angular velocity in degrees per second.
 * @param [in]  duration       Duration (ms) from start to becoming stationary again.
 * @param [in]  on_completion  What to do when the duration completes.
 * @return                     Error code.
 */
pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, uint32_t duration, pbio_control_on_completion_t on_completion);

/**
 * Runs the servo at a given speed by a given angle and stops there.
 *
 * The following convention is used for speed and angle signs:
 *
 *    Speed (+) with angle (+) gives forward (+)
 *    Speed (+) with angle (-) gives backward (-)
 *    Speed (-) with angle (+) gives backward (-)
 *    Speed (-) with angle (-) gives forward (+)
 *
 * @param [in]  srv            The control instance.
 * @param [in]  speed          Top angular velocity in degrees per second. If zero, servo is stopped.
 * @param [in]  angle          Angle to run by.
 * @param [in]  on_completion  What to do after becoming stationary at the final angle.
 * @return                     Error code.
 */
pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_control_on_completion_t on_completion);

/**
 * Runs the servo at a given speed to a given target angle and stops there.
 *
 * The speed sign is ignored. It always goes in the direction needed to
 * read the @p target angle.
 *
 * @param [in]  srv            The control instance.
 * @param [in]  speed          Top angular velocity in degrees per second. If zero, servo is stopped.
 * @param [in]  target         Angle to run to.
 * @param [in]  on_completion  What to do after becoming stationary at the target angle.
 * @return                     Error code.
 */
pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_control_on_completion_t on_completion);

/**
 * Steers the servo to the given target and holds it there.
 *
 * This is similar to pbio_servo_run_target when using hold on completion,
 * but it skips the smooth speed curve and immediately sets the reference
 * angle to the new target.
 *
 * @param [in]  srv            The control instance.
 * @param [in]  target         Angle to run to and keep tracking.
 * @return                     Error code.
 */
pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target);

/**
 * Actuates the servo with a given control type and payload.
 *
 * This is an internal function used after servo or drive base control updates
 * and should not be called directly from external code.
 *
 * @param [in]  srv             The servo instance.
 * @param [in]  actuation_type  The type of actuation to apply.
 * @param [in]  payload         The control payload, such as the amount of torque.
 * @return                      Error code.
 */
pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_dcmotor_actuation_t actuation_type, int32_t payload);

/**
 * Updates the servo state and controller.
 *
 * This gets called once on every control loop.
 */
void pbio_servo_update_all(void);

#endif // PBIO_CONFIG_SERVO

#endif // _PBIO_SERVO_H_

/** @} */

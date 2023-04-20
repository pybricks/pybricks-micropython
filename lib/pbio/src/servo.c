// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <pbdrv/clock.h>
#include <pbdrv/counter.h>
#include <pbdrv/ioport.h>

#include <pbio/angle.h>
#include <pbio/int_math.h>
#include <pbio/observer.h>
#include <pbio/parent.h>
#include <pbio/servo.h>

#if PBIO_CONFIG_SERVO

// Servo motor objects
static pbio_servo_t servos[PBIO_CONFIG_SERVO_NUM_DEV];

/**
 * Gets pointer to static servo instance using port id.
 *
 * @param [in]  port        Port identifier.
 * @param [out] srv         Pointer to servo object.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_get_servo(pbio_port_id_t port, pbio_servo_t **srv) {

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Get address of static servo object.
    *srv = &servos[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    // Get dcmotor object, without additional setup.
    pbio_error_t err = pbio_tacho_get_tacho(port, &((*srv)->tacho));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Get tacho object, without additional setup.
    return pbio_dcmotor_get_dcmotor(port, &((*srv)->dcmotor));
}

static void pbio_servo_update_loop_set_state(pbio_servo_t *srv, bool update) {
    srv->run_update_loop = update;
}

/**
 * Gets the state of the servo update loop.
 *
 * This becomes true after a successful call to pbio_servo_setup and becomes
 * false when there is an error. Such as when the cable is unplugged.
 *
 * @param [in]  srv         The servo instance
 * @return                  True if up and running, false if not.
 */
bool pbio_servo_update_loop_is_running(pbio_servo_t *srv) {

    // Servo must be the parent of its dc motor.
    if (!pbio_parent_equals(&srv->dcmotor->parent, srv)) {
        pbio_servo_update_loop_set_state(srv, false);
    }

    return srv->run_update_loop;
}

static pbio_error_t pbio_servo_update(pbio_servo_t *srv) {

    // Get current time
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state
    pbio_control_state_t state;
    pbio_error_t err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Trajectory reference point
    pbio_trajectory_reference_t ref;

    // Control action to be calculated
    int32_t feedback_torque = 0;
    int32_t feedforward_torque = 0;

    // Check if a control update is needed
    if (pbio_control_is_active(&srv->control)) {

        // Calculate feedback control signal
        pbio_dcmotor_actuation_t requested_actuation;
        bool external_pause = false;
        pbio_control_update(&srv->control, time_now, &state, &ref, &requested_actuation, &feedback_torque, &external_pause);

        // Get required feedforward torque for current reference
        feedforward_torque = pbio_observer_get_feedforward_torque(srv->observer.model, ref.speed, ref.acceleration);

        // Actuate the servo. For torque control, the torque payload is passed along. Otherwise payload is ignored.
        err = pbio_servo_actuate(srv, requested_actuation, feedback_torque + feedforward_torque);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    // Whether or not there is control, get the ongoing actuation state so we can log it and update observer.
    pbio_dcmotor_actuation_t applied_actuation;
    int32_t voltage;
    pbio_dcmotor_get_state(srv->dcmotor, &applied_actuation, &voltage);

    // Optionally log servo state.
    if (pbio_logger_is_active(&srv->log)) {

        // Get stall state
        bool stalled;
        uint32_t stall_duration;
        pbio_servo_is_stalled(srv, &stalled, &stall_duration);

        int32_t log_data[] = {
            // Column 0: Log time (added by logger).
            // Column 1: Current time.
            time_now,
            // Column 2: Motor angle in degrees.
            pbio_control_settings_ctl_to_app_long(&srv->control.settings, &state.position),
            // Column 3: Motor speed in degrees/second.
            pbio_control_settings_ctl_to_app(&srv->control.settings, state.speed),
            // Column 4: Actuation type (LSB 0--1), stall state (LSB 2).
            applied_actuation | ((int32_t)stalled << 2),
            // Column 5: Actuation voltage.
            voltage,
            // Column 6: Estimated position in degrees.
            pbio_control_settings_ctl_to_app_long(&srv->control.settings, &state.position_estimate),
            // Column 7: Estimated speed in degrees/second.
            pbio_control_settings_ctl_to_app(&srv->control.settings, state.speed_estimate),
            // Column 8: Feedback torque (uNm).
            feedback_torque,
            // Column 9: Feedforward torque (uNm).
            feedforward_torque,
            // Column 10: Observer error feedback voltage torque (mV).
            pbio_observer_get_feedback_voltage(&srv->observer, &state.position),
        };
        pbio_logger_add_row(&srv->log, log_data);
    }

    // Update the state observer
    pbio_observer_update(&srv->observer, time_now, &state.position, applied_actuation, voltage);

    return PBIO_SUCCESS;
}

/**
 * Updates the servo state and controller.
 *
 * This gets called once on every control loop.
 */
void pbio_servo_update_all(void) {
    pbio_error_t err;

    // Go through all motors.
    for (uint8_t i = 0; i < PBIO_CONFIG_SERVO_NUM_DEV; i++) {
        pbio_servo_t *srv = &servos[i];

        // Run update loop only if registered.
        if (srv->run_update_loop) {
            err = pbio_servo_update(srv);
            if (err != PBIO_SUCCESS) {
                // If the update failed, don't update it anymore.
                pbio_servo_update_loop_set_state(srv, false);

                // Coast the motor, letting errors pass.
                pbio_dcmotor_coast(srv->dcmotor);

                // Stop the control state.
                pbio_control_reset(&srv->control);

                // Stop higher level controls, such as drive bases.
                pbio_parent_stop(&srv->parent, false);
            }
        }
    }
}

// This function is attached to a dcmotor object, so it is able to
// stop the servo if the dcmotor needs to execute a new command.
static pbio_error_t pbio_servo_stop_from_dcmotor(void *servo, bool clear_parent) {

    // Specify pointer type.
    pbio_servo_t *srv = servo;

    // This external stop is triggered by a lower level peripheral,
    // i.e. the dc motor. So it has already has been stopped or changed state
    // electrically. All we have to do here is stop the control loop,
    // so it won't override the dcmotor to do something else.
    if (pbio_control_is_active(&srv->control)) {
        pbio_control_reset(&srv->control);

        // If we're not clearing the parent, we are done here. We don't want
        // to keep calling the drive base stop over and over.
        if (!clear_parent) {
            return PBIO_SUCCESS;
        }
    }

    // If servo control wasn't active, it's still possible that a higher
    // level abstraction is using this device. So call its stop as well.
    return pbio_parent_stop(&srv->parent, clear_parent);
}

#define DEG_TO_MDEG(deg) ((deg) * 1000)

/**
 * Loads all parameters of a servo to make it ready for use.
 *
 * @param [in]    srv                The servo instance.
 * @param [in]    gear_ratio         Ratio that converts control units (mdeg) to user-defined output units (e.g. deg).
 * @param [in]    precision_profile  Position tolerance around target in degrees. Set to 0 to load default profile for this motor.
 * @return                           Error code.
 */
static pbio_error_t pbio_servo_initialize_settings(pbio_servo_t *srv, int32_t gear_ratio, int32_t precision_profile) {

    // Get the device type to load relevant settings.
    pbio_iodev_type_id_t type_id;
    pbio_error_t err = pbdrv_ioport_get_motor_device_type_id(srv->dcmotor->port, &type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Gear ratio must be strictly positive.
    if (gear_ratio < 1) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Get the minimal set of defaults for this servo type.
    const pbio_servo_settings_reduced_t *settings_reduced = pbio_servo_get_reduced_settings(type_id);
    if (!settings_reduced) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    // If precision not given (if 0), use default value for this motor.
    if (precision_profile == 0) {
        precision_profile = settings_reduced->precision_profile;
    }

    // The tighter the tolerances, the higher the feedback values. This enforces
    // an upper limit on the feedback gains.
    if (precision_profile < 5) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Save reference to motor model.
    srv->observer.model = settings_reduced->model;

    // Initialize maximum torque as the stall torque for maximum voltage.
    // In practice, the nominal voltage is a bit lower than the 9V values.
    // REVISIT: Select nominal voltage based on battery type instead of 7500.
    int32_t max_voltage = pbio_dcmotor_get_max_voltage(type_id);
    int32_t nominal_voltage = pbio_int_math_min(max_voltage, 7500);
    int32_t nominal_torque = pbio_observer_voltage_to_torque(srv->observer.model, nominal_voltage);

    // Set all control settings.
    srv->control.settings = (pbio_control_settings_t) {
        // For a servo, counts per output unit is counts per degree at the gear train output
        .ctl_steps_per_app_step = gear_ratio,
        .stall_speed_limit = DEG_TO_MDEG(20),
        .stall_time = pbio_control_time_ms_to_ticks(200),
        .speed_max = DEG_TO_MDEG(settings_reduced->rated_max_speed),
        // The default speed is not used for servos currently (an explicit speed
        // is given for all run commands), so we initialize it to the maximum.
        .speed_default = DEG_TO_MDEG(settings_reduced->rated_max_speed),
        .speed_tolerance = DEG_TO_MDEG(50),
        .position_tolerance = DEG_TO_MDEG(precision_profile),
        .acceleration = DEG_TO_MDEG(2000),
        .deceleration = DEG_TO_MDEG(2000),
        .actuation_max = pbio_observer_voltage_to_torque(srv->observer.model, max_voltage),
        // The nominal voltage is an indication for the nominal torque limit. To
        // ensure proportional control can always get the motor to within the
        // configured tolerance, we select pid_kp such that proportional feedback
        // just exceeds the nominal torque at the tolerance boundary.
        .pid_kp = nominal_torque / precision_profile,
        // Initialize ki such that integral control saturates in about two seconds
        // if the motor were stuck at the position tolerance.
        .pid_ki = nominal_torque / precision_profile / 2,
        // By default, the kd value is the same ratio of kp on all motors to
        // get a comparable step response. This value is not reduced along with
        // the user given precision profile like kp, so we use the default
        // value from the reduced settings.
        .pid_kd = nominal_torque / settings_reduced->precision_profile / 8,
        .pid_kp_low_pct = 50,
        .pid_kp_low_error_threshold = DEG_TO_MDEG(5),
        .pid_kp_low_speed_threshold = DEG_TO_MDEG(settings_reduced->pid_kp_low_speed_threshold),
        .integral_deadzone = DEG_TO_MDEG(8),
        .integral_change_max = DEG_TO_MDEG(15),
        .smart_passive_hold_time = pbio_control_time_ms_to_ticks(100),
    };

    // Initialize all observer settings.
    srv->observer.settings = (pbio_observer_settings_t) {
        .stall_speed_limit = srv->control.settings.stall_speed_limit,
        .stall_time = srv->control.settings.stall_time,
        .feedback_voltage_negligible = pbio_observer_torque_to_voltage(srv->observer.model, srv->observer.model->torque_friction) * 5 / 2,
        .feedback_voltage_stall_ratio = 75,
        .feedback_gain_low = settings_reduced->feedback_gain_low,
        .feedback_gain_high = settings_reduced->feedback_gain_low * 7,
        .feedback_gain_threshold = DEG_TO_MDEG(20),
        .coulomb_friction_speed_cutoff = 500,
    };

    return PBIO_SUCCESS;
}

/**
 * Sets up the servo instance to be used in an application.
 *
 * @param [in]  srv               The servo instance.
 * @param [in]  direction         The direction of positive rotation.
 * @param [in]  gear_ratio        The ratio between motor rotation (millidegrees) and the gear train output (degrees).
 * @param [in]  reset_angle       If true, reset the current angle to the current absolute position if supported or 0.
 * @param [in]  precision_profile Position tolerance around target in degrees. Set to 0 to load default profile for this motor.
 * @return                        Error code.
 */
pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, int32_t gear_ratio, bool reset_angle, int32_t precision_profile) {
    pbio_error_t err;

    // Unregister this servo from control loop updates.
    pbio_servo_update_loop_set_state(srv, false);

    // Configure tacho.
    err = pbio_tacho_setup(srv->tacho, direction, reset_angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Coast and configure dcmotors, and stop its parents, if any.
    err = pbio_dcmotor_setup(srv->dcmotor, direction);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // This servo will be the parent of this dcmotor
    pbio_parent_set(&srv->dcmotor->parent, srv, pbio_servo_stop_from_dcmotor);

    // Reset state
    pbio_control_reset(&srv->control);

    // Load default settings for this device type.
    err = pbio_servo_initialize_settings(srv, gear_ratio, precision_profile);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get current angle.
    pbio_angle_t angle;
    err = pbio_tacho_get_angle(srv->tacho, &angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reset observer to current angle.
    pbio_observer_reset(&srv->observer, &angle);

    // Now that all checks have succeeded, we know that this motor is ready.
    // So we register this servo from control loop updates.
    pbio_servo_update_loop_set_state(srv, true);

    return PBIO_SUCCESS;
}

/**
 * Resets the servo angle to a given value.
 *
 * @param [in]  srv          The servo instance.
 * @param [in]  reset_angle  Angle that servo should now report in degrees.
 * @param [in]  reset_to_abs If true, ignores reset_angle and resets to absolute angle marked on shaft instead.
 * @return                   Error code.
 */
pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {

    // If we were busy moving or holding position, that means the reset was
    // called while a controller was running in the background. To avoid
    // confusion as to where the motor must go after the reset, we'll make it
    // stop and apply the configured stop mode right away.
    bool apply_stop = pbio_control_is_active(&srv->control);
    pbio_control_on_completion_t on_completion = srv->control.on_completion;

    // Get the current state so we can restore it after resetting if needed.
    pbio_dcmotor_actuation_t actuation;
    int32_t voltage;
    pbio_dcmotor_get_state(srv->dcmotor, &actuation, &voltage);

    // Stop servo and parents, if any.
    pbio_error_t err = pbio_servo_stop(srv, PBIO_CONTROL_ON_COMPLETION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reset control state, including persistent on completion type.
    pbio_control_reset(&srv->control);

    // Get new angle in state units.
    pbio_angle_t new_angle;
    pbio_control_settings_app_to_ctl_long(&srv->control.settings, reset_angle, &new_angle);

    // Reset the tacho to the new angle. If reset_to_abs is true, the
    // new_angle will be an output, representing the angle it was set to.
    // This lets us use it again to reset the observer below.
    err = pbio_tacho_reset_angle(srv->tacho, &new_angle, reset_to_abs);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reset observer to new angle.
    pbio_observer_reset(&srv->observer, &new_angle);

    // Apply user selected completion mode on stop if control was active during reset.
    if (apply_stop) {
        return pbio_servo_stop(srv, on_completion);
    }

    // Otherwise, restore brake or passive voltage.
    if (actuation == PBIO_DCMOTOR_ACTUATION_VOLTAGE) {
        return pbio_dcmotor_set_voltage(srv->dcmotor, voltage);
    }

    // Otherwise, we were coasting, so keep doing that.
    return PBIO_SUCCESS;
}

/**
 * Gets the servo state in units of control. This means millidegrees at the
 * motor output shaft, before any external gearing.
 *
 * @param [in]  srv         The servo instance.
 * @param [out] state       The system state object in units of control.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_get_state_control(pbio_servo_t *srv, pbio_control_state_t *state) {

    pbio_error_t err;

    // Read physical angle.
    err = pbio_tacho_get_angle(srv->tacho, &state->position);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get estimated state
    pbio_observer_get_estimated_state(&srv->observer, &state->speed, &state->position_estimate, &state->speed_estimate);

    return PBIO_SUCCESS;
}

/**
 * Gets the servo state in units of degrees at the output.
 *
 * @param [in]  srv         The servo instance.
 * @param [out] angle       Angle in degrees.
 * @param [out] speed       Angular velocity in degrees per second.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_get_state_user(pbio_servo_t *srv, int32_t *angle, int32_t *speed) {

    // Don't allow user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get servo state.
    pbio_control_state_t state;
    pbio_error_t err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Scale by gear ratio to whole degrees.
    *angle = pbio_control_settings_ctl_to_app_long(&srv->control.settings, &state.position);
    *speed = pbio_control_settings_ctl_to_app(&srv->control.settings, state.speed);
    return PBIO_SUCCESS;
}

/**
 * Gets the servo speed in units of degrees per second at the output, with
 * a given window size to control how smooth the speed differentiation is.
 *
 * @param [in]  srv         The servo instance.
 * @param [in]  window      Window size in milliseconds.
 * @param [out] speed       Calculated speed in degrees per second.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_get_speed_user(pbio_servo_t *srv, uint32_t window, int32_t *speed) {
    pbio_error_t err = pbio_differentiator_get_speed(&srv->observer.differentiator, window, speed);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *speed = pbio_control_settings_ctl_to_app(&srv->control.settings, *speed);
    return PBIO_SUCCESS;
}

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
pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_dcmotor_actuation_t actuation_type, int32_t payload) {

    // Apply the calculated actuation, by type.
    switch (actuation_type) {
        case PBIO_DCMOTOR_ACTUATION_COAST:
            return pbio_dcmotor_coast(srv->dcmotor);
        case PBIO_DCMOTOR_ACTUATION_BRAKE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, 0);
        case PBIO_DCMOTOR_ACTUATION_VOLTAGE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, payload);
        case PBIO_DCMOTOR_ACTUATION_TORQUE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, pbio_observer_torque_to_voltage(srv->observer.model, payload));
        default:
            return PBIO_ERROR_INVALID_ARG;
    }
}

/**
 * Stops ongoing controlled motion to coast, brake, or hold the servo.
 *
 * @param [in]  srv           The servo instance.
 * @param [in]  on_completion Coast, brake, or hold after stopping the controller.
 * @return                    Error code.
 */
pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Handle HOLD case. Also enforce hold if the stop type was CONTINUE since
    // this function needs to make it stop in all cases.
    if (on_completion == PBIO_CONTROL_ON_COMPLETION_HOLD ||
        on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE) {

        // Get current physical and estimated state.
        pbio_control_state_t state;
        err = pbio_servo_get_state_control(srv, &state);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // To hold, we first have to figure out which angle to hold.
        const pbio_angle_t *hold_target;
        pbio_trajectory_reference_t ref;
        if (pbio_control_is_active(&srv->control)) {
            // If control is active, hold at current target, so get it.
            uint32_t time = pbio_control_get_time_ticks();
            pbio_control_get_reference(&srv->control, time, &state, &ref);
            hold_target = &ref.position;
        } else {
            // If no control is ongoing, just hold measured state.
            hold_target = &state.position;
        }
        // Track the hold angle.
        return pbio_servo_track_target(srv, pbio_control_settings_ctl_to_app_long(&srv->control.settings, hold_target));
    }

    // All other stop modes are passive, so stop control and actuate accordingly.
    pbio_control_stop(&srv->control);
    return pbio_servo_actuate(srv, pbio_control_passive_completion_to_actuation_type(on_completion), 0);
}

/**
 * Runs the servo at a given speed and stops after a given duration or runs forever.
 *
 * @param [in]  srv            The servo instance.
 * @param [in]  speed          Angular velocity in degrees per second.
 * @param [in]  duration       Duration (ms) from start to becoming stationary again.
 * @param [in]  on_completion  What to do when the duration completes.
 * @return                     Error code.
 */
static pbio_error_t pbio_servo_run_time_common(pbio_servo_t *srv, int32_t speed, uint32_t duration, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get current time
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state
    pbio_control_state_t state;
    err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver.
    return pbio_control_start_timed_control(&srv->control, time_now, &state, duration, speed, on_completion);
}

/**
 * Starts running the servo at a given speed.
 *
 * @param [in]  srv             The servo instance.
 * @param [in]  speed           Angular velocity in degrees per second.
 * @return                      Error code.
 */
pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed) {
    // Start a timed maneuver and restart it when it is done, thus running forever.
    return pbio_servo_run_time_common(srv, speed, PBIO_TRAJECTORY_DURATION_FOREVER_MS, PBIO_CONTROL_ON_COMPLETION_CONTINUE);
}

/**
 * Runs the servo at a given speed and stops after a given duration.
 *
 * @param [in]  srv            The servo instance.
 * @param [in]  speed          Angular velocity in degrees per second.
 * @param [in]  duration       Duration (ms) from start to becoming stationary again.
 * @param [in]  on_completion  What to do when the duration completes.
 * @return                     Error code.
 */
pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, uint32_t duration, pbio_control_on_completion_t on_completion) {
    // Start a timed maneuver, duration specified by user.
    return pbio_servo_run_time_common(srv, speed, duration, on_completion);
}

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
pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get current time
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state
    pbio_control_state_t state;
    err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If the speed is zero, we can't do anything meaningful, but most users
    // find it confusing if we return an error. To make sure it won't block
    // forever, we run by a relative angle to zero, which is immediately done.
    if (speed == 0) {
        return pbio_servo_run_angle(srv, speed, 0, on_completion);
    }


    return pbio_control_start_position_control(&srv->control, time_now, &state, target, speed, on_completion);
}

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
pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get current time.
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state.
    pbio_control_state_t state;
    err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If the speed is zero, we can't do anything meaningful, but most users
    // find it confusing if we return an error. To make sure it won't block
    // forever, we set the angle to zero instead, so we're "done" right away.
    if (speed == 0) {
        angle = 0;
    }

    // Start the relative angle control
    return pbio_control_start_position_control_relative(&srv->control, time_now, &state, angle, speed, on_completion, true);
}

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
pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start hold command.
    return pbio_control_start_position_control_hold(&srv->control, pbio_control_get_time_ticks(), target);
}

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
pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled, uint32_t *stall_duration) {

    // Don't allow access if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        *stalled = false;
        *stall_duration = 0;
        return PBIO_ERROR_INVALID_OP;
    }

    // If control is active, this provides the most accurate stall detection.
    if (pbio_control_is_active(&srv->control)) {
        *stalled = pbio_control_is_stalled(&srv->control, stall_duration);
        *stall_duration = pbio_control_time_ticks_to_ms(*stall_duration);
        return PBIO_SUCCESS;
    }

    // If we're coasting, we're not stalled.
    pbio_dcmotor_actuation_t actuation;
    int32_t voltage;
    pbio_dcmotor_get_state(srv->dcmotor, &actuation, &voltage);
    if (actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        *stall_duration = 0;
        *stalled = false;
        return PBIO_SUCCESS;
    }

    // If we're here, the user has set their own voltage or duty cycle value.
    // In this case, the best we can do is ask the observer if we're stuck.
    *stalled = pbio_observer_is_stalled(&srv->observer, pbio_control_get_time_ticks(), stall_duration);
    *stall_duration = pbio_control_time_ticks_to_ms(*stall_duration);

    return PBIO_SUCCESS;
}

/**
 * Gets estimated external load experienced by the servo.
 *
 * @param [in]  srv     The servo instance.
 * @param [out] load    Estimated load (mNm).
 * @return              Error code.
 */
pbio_error_t pbio_servo_get_load(pbio_servo_t *srv, int32_t *load) {

    // Don't allow access if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        *load = 0;
        return PBIO_ERROR_INVALID_OP;
    }

    // Get passive actuation type.
    pbio_dcmotor_actuation_t applied_actuation;
    int32_t voltage;
    pbio_dcmotor_get_state(srv->dcmotor, &applied_actuation, &voltage);

    // Get best estimate based on control and physyical state.
    if (applied_actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        // Can't estimate load on coast.
        *load = 0;
    } else if (pbio_control_is_active(&srv->control)) {
        // The experienced load is the opposite sign of what the PID is
        // trying to overcome.
        *load = -srv->control.pid_average;
    } else {
        // Read the angle.
        pbio_angle_t angle;
        pbio_error_t err = pbio_tacho_get_angle(srv->tacho, &angle);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Use observer error as a measure of torque.
        int32_t feedback_voltage = pbio_observer_get_feedback_voltage(&srv->observer, &angle);
        *load = pbio_observer_voltage_to_torque(srv->observer.model, feedback_voltage);
    }

    // Convert to user torque units (mNm).
    *load = pbio_control_settings_actuation_ctl_to_app(*load);

    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_SERVO

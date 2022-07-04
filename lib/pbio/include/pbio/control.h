// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef _PBIO_CONTROL_H_
#define _PBIO_CONTROL_H_

#include <stdint.h>

#include <pbio/angle.h>
#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/dcmotor.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>
#include <pbio/logger.h>

#include <pbio/iodev.h>

#define PBIO_CONTROL_LOG_COLS (13)

/**
 * Control settings.
 */
typedef struct _pbio_control_settings_t {
    /**
     * The amount of "position-like control steps" for each "application step".
     *
     * This is used to scale practical end-user units (e.g. mm or deg) to
     * internal high resolution control units (e.g. mdeg).
     *
     * Each application (e.g. a servo with external gearing) defines its own
     * control units, application units, and the scaling factor to match.
     *
     * For example, in a servo with an external gear ratio of 5 with outputs in
     * degrees, and internal control in millidegrees, this value is 5000.
     *
     * All attributes given below are expressed in control units and control
     * ticks for time.
     */
    int32_t ctl_steps_per_app_step;
    /**
     * If this speed cannot be reached even with the maximum control signal
     * defined in actuation_max, the controller is stalled.
     */
    int32_t stall_speed_limit;
    /**
     * Minimum consequtive stall time before stall flag getter returns true.
     */
    uint32_t stall_time;
    /**
     * Speed that the user input will be capped to.
     */
    int32_t speed_max;
    /**
     * Speed used for calls without speed arguments.
     */
    int32_t speed_default;
    /**
     * Allowed speed deviation for controller to be on target. If the target
     * speed is zero such as at the end of the maneuver, this essentially sets
     * the speed threshold below which the controller is considered stationary.
     */
    int32_t speed_tolerance;
    /**
     * Allowed position deviation for controller to be on target.
     */
    int32_t position_tolerance;
    /**
     * Absolute rate of change of the speed during on-ramp of the maneuver.
     * This value will be used for the on-ramp even if the application
     * initially starts above the target speed, meaning it is decelerating.
     */
    int32_t acceleration;
    /**
     * Absolute rate of change of the speed during off-ramp of the maneuver.
     */
    int32_t deceleration;
    /**
     * Maximum feedback actuation value. On a motor this is the maximum torque.
     */
    int32_t actuation_max;
    /**
     * Position error feedback constant.
     */
    int32_t pid_kp;
    /**
     * Accumulated position error feedback constant.
     */
    int32_t pid_ki;
    /**
     * Speed error feedback constant.
     */
    int32_t pid_kd;
    /**
     * Absolute bound on the rate at which the integrator accumulates errors.
     */
    int32_t integral_change_max;
} pbio_control_settings_t;

/**
 * Actions to be taken when a control command completes.
 */
typedef enum {
    /** On completion, coast the motor and reset control state. */
    PBIO_CONTROL_ON_COMPLETION_COAST = PBIO_DCMOTOR_ACTUATION_COAST,
    /** On completion, brake the motor and reset control state. */
    PBIO_CONTROL_ON_COMPLETION_BRAKE = PBIO_DCMOTOR_ACTUATION_BRAKE,
    /** On completion, actively hold the motor in place */
    PBIO_CONTROL_ON_COMPLETION_HOLD,
    /** On completion, keep moving at target speed */
    PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    /** On completion, coast the motor, but use endpoint of this maneuver
     * as starting point for the next relative angle maneuver. */
    PBIO_CONTROL_ON_COMPLETION_COAST_SMART,
} pbio_control_on_completion_t;

/**
 * State of a system that is being controlled.
 */
typedef struct _pbio_control_state_t {
    /**
     * Position of the system. Although position does not necessarily represent
     * an angle, we use the angle type here since it can hold a large range
     * of values, and because single motors can just copy their measured state
     * without further scaling. For drivebases, this can be the "heading" or
     * "distance" driven.
     */
    pbio_angle_t position;
    /**
     * Estimated position from application-specific state observer.
     */
    pbio_angle_t position_estimate;
    /**
     * Estimated speed from application-specific state observer.
     */
    int32_t speed_estimate;
} pbio_control_state_t;

/**
 * Type of controller that is currently active.
 */
typedef enum {
    /**
     * No control is active.
     */
    PBIO_CONTROL_NONE,
    /**
     * Run at a given speed for a given amount of time. The exact position
     * during and after the maneuver is not important. This uses PI control
     * on the speed error, which is implemented as PD control on a position
     * signal whose reference pauses when it is blocked.
     */
    PBIO_CONTROL_TIMED,
    /**
     * Run run at a given speed to a given position, however long it takes. This
     * uses classical PID control, except that D uses the estimated speed. It
     * uses anti-windup schemes to prevent P and I from growing when blocked.
     */
    PBIO_CONTROL_POSITION,
} pbio_control_type_t;

/**
 * Controller status and state.
 */
typedef struct _pbio_control_t {
    /**
     * The type of controller that is currently active.
     */
    pbio_control_type_t type;
    /**
     * Configurable control settings.
     */
    pbio_control_settings_t settings;
    /**
     * Action to be taken when current command completes.
     */
    pbio_control_on_completion_t on_completion;
    /**
     * Collection of 4 points that define the trajectory that the controller
     * follows. If the controller is no longer active, this still holds the
     * last-used trajectory.
     */
    pbio_trajectory_t trajectory;
    /**
     * Integrator of the speed error. Used when timed speed control is active.
     */
    pbio_speed_integrator_t speed_integrator;
    /**
     * Integrator of the position error. Used when angle control is active.
     */
    pbio_position_integrator_t position_integrator;
    /**
     * Structure with data log settings and pointer to data buffer if active.
     */
    pbio_log_t log;
    /**
     * Slow moving average of the PID output, which is a measure for load.
     */
    int32_t load;
    /**
     * Flag that says whether the controller is currently stalled.
     */
    bool stalled;
    /**
     * Flag that says whether the controller is currently on target, as given
     * by the endpoint of the trajectory that is being followed.
     */
    bool on_target;
} pbio_control_t;

// Time functions.
uint32_t pbio_control_get_time_ticks(void);
uint32_t pbio_control_get_ref_time(pbio_control_t *ctl, uint32_t time_now);
bool pbio_control_time_is_later(uint32_t sample, uint32_t base);

/**
 * Converts milliseconds to time ticks used by controller.
 *
 * @param [in] ms             Time in milliseconds.
 * @return                    Time converted to control ticks.
 */
#define pbio_control_time_ms_to_ticks(ms) ((ms) * 10)

/**
 * Converts time ticks used by controller to milliseconds.
 *
 * @param [in] ticks          Control timer ticks.
 * @return                    Time converted to milliseconds.
 */
#define pbio_control_time_ticks_to_ms(ticks) ((ticks) / 10)

// Unit conversion functions.
int32_t pbio_control_position_ctl_to_app(pbio_control_settings_t *s, int32_t input);
int32_t pbio_control_position_ctl_to_app_long(pbio_control_settings_t *s, pbio_angle_t *input);
int32_t pbio_control_position_app_to_ctl(pbio_control_settings_t *s, int32_t input);
void pbio_control_position_app_to_ctl_long(pbio_control_settings_t *s, int32_t input, pbio_angle_t *output);
int32_t pbio_control_actuation_ctl_to_app(int32_t input);
int32_t pbio_control_actuation_app_to_ctl(int32_t input);

// Control settings getters and setters.
void pbio_control_settings_get_limits(pbio_control_settings_t *s, int32_t *speed, int32_t *acceleration, int32_t *deceleration, int32_t *actuation);
pbio_error_t pbio_control_settings_set_limits(pbio_control_settings_t *s, int32_t speed, int32_t acceleration, int32_t deceleration, int32_t actuation);
void pbio_control_settings_get_pid(pbio_control_settings_t *s, int32_t *pid_kp, int32_t *pid_ki, int32_t *pid_kd, int32_t *integral_change_max);
pbio_error_t pbio_control_settings_set_pid(pbio_control_settings_t *s, int32_t pid_kp, int32_t pid_ki, int32_t pid_kd, int32_t integral_change_max);
void pbio_control_settings_get_target_tolerances(pbio_control_settings_t *s, int32_t *speed, int32_t *position);
pbio_error_t pbio_control_settings_set_target_tolerances(pbio_control_settings_t *s, int32_t speed, int32_t position);
void pbio_control_settings_get_stall_tolerances(pbio_control_settings_t *s,  int32_t *speed, uint32_t *time);
pbio_error_t pbio_control_settings_set_stall_tolerances(pbio_control_settings_t *s, int32_t speed, uint32_t time);

// Control loop functions.
void pbio_control_reset(pbio_control_t *ctl);
void pbio_control_stop(pbio_control_t *ctl);
void pbio_control_update(pbio_control_t *ctl, uint32_t time_now, pbio_control_state_t *state, pbio_trajectory_reference_t *ref, pbio_dcmotor_actuation_t *actuation, int32_t *control);

// Control status checks.
bool pbio_control_is_active(pbio_control_t *ctl);
bool pbio_control_type_is_position(pbio_control_t *ctl);
bool pbio_control_type_is_time(pbio_control_t *ctl);
bool pbio_control_is_stalled(pbio_control_t *ctl, uint32_t *stall_duration);
bool pbio_control_is_done(pbio_control_t *ctl);
int32_t pbio_control_get_load(pbio_control_t *ctl);

// Start new control command.
pbio_error_t pbio_control_start_position_control(pbio_control_t *ctl, uint32_t time_now, pbio_control_state_t *state, int32_t position, int32_t speed, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_control_start_position_control_relative(pbio_control_t *ctl, uint32_t time_now, pbio_control_state_t *state, int32_t distance, int32_t speed, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_control_start_position_control_hold(pbio_control_t *ctl, uint32_t time_now, int32_t position);
pbio_error_t pbio_control_start_timed_control(pbio_control_t *ctl, uint32_t time_now, pbio_control_state_t *state, int32_t duration, int32_t speed, pbio_control_on_completion_t on_completion);

#endif // _PBIO_CONTROL_H_

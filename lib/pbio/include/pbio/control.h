// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

/**
 * @addtogroup Control pbio/control: PID-like control
 *
 * Provides application-agnostic controllers for fully-actuated mechanical systems.
 * @{
 */

#ifndef _PBIO_CONTROL_H_
#define _PBIO_CONTROL_H_

#include <stdint.h>

#include <pbio/angle.h>
#include <pbio/control_settings.h>
#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/dcmotor.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>
#include <pbio/logger.h>

// Number of values per row when control data logger is active.
#define PBIO_CONTROL_LOGGER_NUM_COLS (12)

/**
 * Actions to be taken when a control command completes.
 */
typedef enum {
    /** On completion, passively coast the motor and reset control state. */
    PBIO_CONTROL_ON_COMPLETION_COAST,
    /** On completion, passively brake the motor and reset control state. */
    PBIO_CONTROL_ON_COMPLETION_BRAKE,
    /** On completion, actively hold the motor in place */
    PBIO_CONTROL_ON_COMPLETION_HOLD,
    /** On completion, actively keep moving at target speed */
    PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    /**
     * A short while after completion, coast the motor, and use endpoint of
     * this maneuver as starting point for the next relative angle maneuver.
     */
    PBIO_CONTROL_ON_COMPLETION_COAST_SMART,
    /**
     * A short while after completion, brake the motor, and use endpoint of
     * this maneuver as starting point for the next relative angle maneuver.
     */
    PBIO_CONTROL_ON_COMPLETION_BRAKE_SMART,
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
     * Speed determined as numerical derivative of position.
     */
    int32_t speed;
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
    PBIO_CONTROL_TYPE_NONE = 0,
    /**
     * Run at a given speed for a given amount of time. The exact position
     * during and after the maneuver is not important. This uses PI control
     * on the speed error, which is implemented as PD control on a position
     * signal whose reference pauses when it is blocked.
     */
    PBIO_CONTROL_TYPE_TIMED = 1,
    /**
     * Run run at a given speed to a given position, however long it takes. This
     * uses classical PID control, except that D uses the estimated speed. It
     * uses anti-windup schemes to prevent P and I from growing when blocked.
     */
    PBIO_CONTROL_TYPE_POSITION = 2,
    /**
     * Mask to extract the type of controller from the control type.
     */
    PBIO_CONTROL_TYPE_MASK = 3,
    /**
     * Flag indicating that controller should stop (complete) when it stalls.
     * Can be used to have an always-active stall protection.
     */
    PBIO_CONTROL_TYPE_FLAG_STOP_ON_STALL = 1 << 2,
    /**
     * Flag indicating controller should run until it stalls. Can be used to
     * explicitly run until it stalls to find a mechanism endpoint.
     */
    PBIO_CONTROL_TYPE_FLAG_OBJECTIVE_IS_STALL = 1 << 3,
} pbio_control_type_t;

/**
 * Flags to indicate control status.
 */
typedef enum {
    /** The controller is stalled. */
    PBIO_CONTROL_STATUS_STALLED  = 1 << 0,
    /** The control objective is complete/achieved. */
    PBIO_CONTROL_STATUS_COMPLETE = 1 << 1,
} pbio_control_status_flag_t;

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
    int32_t pid_average;
    /**
     * Control state flags such as being on target and/or being stalled.
     */
    pbio_control_status_flag_t status;
} pbio_control_t;

// Time and reference functions:

uint32_t pbio_control_get_time_ticks(void);
uint32_t pbio_control_get_ref_time(const pbio_control_t *ctl, uint32_t time_now);
void pbio_control_get_reference(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, pbio_trajectory_reference_t *ref);

// Control loop functions:

void pbio_control_reset(pbio_control_t *ctl);
void pbio_control_stop(pbio_control_t *ctl);
void pbio_control_update(
    pbio_control_t *ctl,
    uint32_t time_now,
    const pbio_control_state_t *state,
    pbio_trajectory_reference_t *ref,
    pbio_dcmotor_actuation_t *actuation,
    int32_t *control,
    bool *external_pause);

// Control status checks:

bool pbio_control_is_active(const pbio_control_t *ctl);
bool pbio_control_type_is_position(const pbio_control_t *ctl);
bool pbio_control_type_is_time(const pbio_control_t *ctl);
bool pbio_control_is_stalled(const pbio_control_t *ctl, uint32_t *stall_duration);
bool pbio_control_is_done(const pbio_control_t *ctl);
pbio_dcmotor_actuation_t pbio_control_passive_completion_to_actuation_type(pbio_control_on_completion_t on_completion);

// Start new control command:

pbio_error_t pbio_control_start_position_control(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, int32_t position, int32_t speed, pbio_control_on_completion_t on_completion);
pbio_error_t pbio_control_start_position_control_relative(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, int32_t distance, int32_t speed, pbio_control_on_completion_t on_completion, bool allow_trajectory_shift);
pbio_error_t pbio_control_start_position_control_hold(pbio_control_t *ctl, uint32_t time_now, int32_t position);
pbio_error_t pbio_control_start_timed_control(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, uint32_t duration, int32_t speed, pbio_control_on_completion_t on_completion);

#endif // _PBIO_CONTROL_H_

/** @} */

// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup Trajectory pbio/trajectory: Compute and poll motor trajectories
 *
 * Generate and read trajectories with given speed, acceleration, and endpoint.
 * @{
 */

#ifndef _PBIO_TRAJECTORY_H_
#define _PBIO_TRAJECTORY_H_

#include <stdint.h>

#include <pbio/angle.h>
#include <pbio/error.h>

// Trajectories use sub-millisecond steps for increased resolution.
#define PBIO_TRAJECTORY_TICKS_PER_MS (10)

// The duration argument of infinite maneuvers is essentially irrelevant since
// motion keeps going due to the absence of a deceleration phase. Still, we
// need a nonzero number within which we can find a valid solution for the
// acceleration part of the maneuver.
#define PBIO_TRAJECTORY_DURATION_FOREVER_MS (5 * 60 * 1000)

/**
 * Minimal set of trajectory parameters from which a full trajectory is
 * calculated. All values in control units and time in ticks.
 */
typedef struct _pbio_trajectory_command_t {
    uint32_t time_start;           /**<  Time at start of maneuver */
    pbio_angle_t position_start;   /**<  Position at start of maneuver */
    pbio_angle_t position_end;     /**<  Position at end of maneuver. Only used for position commands. */
    uint32_t duration;             /**<  Duration of maneuver. Only used for timed commands. */
    int32_t speed_start;           /**<  Encoder rate at start of maneuver */
    int32_t speed_target;          /**<  Encoder target rate target when not accelerating */
    int32_t speed_max;             /**<  Max target rate target */
    int32_t acceleration;          /**<  Encoder acceleration magnitude during in-phase */
    int32_t deceleration;          /**<  Encoder acceleration magnitude during out-phase */
    bool continue_running;         /**<  Whether it movement continues after t3 (true) or not (false) */
} pbio_trajectory_command_t;

/**
 * Trajectory evaluated at a given point in time, in control units.
 */
typedef struct _pbio_trajectory_reference_t {
    uint32_t time;          /**<  Reference time */
    pbio_angle_t position;  /**<  Reference position */
    int32_t speed;          /**<  Reference speed */
    int32_t acceleration;   /**<  Reference acceleration */
} pbio_trajectory_reference_t;

/**
 * Complete set of motor trajectory parameters for an ideal maneuver without
 * disturbances. These values have custom units to keep them within safe
 * numerical bounds. Reference getters should be used to get outputs in
 * control units instead.
 */
typedef struct _pbio_trajectory_t {
    pbio_trajectory_reference_t start;   /**<  Starting point of the trajectory. */
    int32_t t1;                          /**<  Signed time from start to end of acceleration phase */
    int32_t t2;                          /**<  Signed time from start to start of deceleration phase */
    int32_t t3;                          /**<  Signed time from start to end of maneuver */
    int32_t th1;                         /**<  Encoder count after the acceleration in-phase */
    int32_t th2;                         /**<  Encoder count at start of acceleration out-phase */
    int32_t th3;                         /**<  Encoder count at end of maneuver */
    int32_t w0;                          /**<  Encoder rate at start of maneuver */
    int32_t w1;                          /**<  Encoder rate target when not accelerating */
    int32_t w3;                          /**<  Encoder rate target after the maneuver ends */
    int32_t a0;                          /**<  Encoder acceleration during in-phase */
    int32_t a2;                          /**<  Encoder acceleration during out-phase */
} pbio_trajectory_t;

// Make or modify trajectories:

pbio_error_t pbio_trajectory_new_angle_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command);
pbio_error_t pbio_trajectory_new_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command);
void pbio_trajectory_make_constant(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command);
void pbio_trajectory_stretch(pbio_trajectory_t *trj, pbio_trajectory_t *leader);

// Reference getter functions:

uint32_t pbio_trajectory_get_duration(pbio_trajectory_t *trj);
void pbio_trajectory_get_endpoint(pbio_trajectory_t *trj, pbio_trajectory_reference_t *end);
void pbio_trajectory_get_last_vertex(pbio_trajectory_t *trj, uint32_t time_ref, pbio_trajectory_reference_t *ref);
void pbio_trajectory_get_reference(pbio_trajectory_t *trj, uint32_t time_ref, pbio_trajectory_reference_t *ref);

#endif // _PBIO_TRAJECTORY_H_

/** @} */

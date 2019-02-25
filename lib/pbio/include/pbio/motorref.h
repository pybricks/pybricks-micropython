// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef _PBIO_MOTORREF_H_
#define _PBIO_MOTORREF_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#include <pbio/motor.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

#define NONE (0) // A "don't care" constant for readibility of the code, but which is never used after assignment
#define NONZERO (100) // Arbitrary nonzero speed

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// Macro to evaluate b*t/US_PER_SECOND in two steps to avoid excessive round-off errors and overflows.
#define timest(b, t) ((b * ((t)/US_PER_MS))/MS_PER_SECOND)
// Same trick to evaluate formulas of the form 1/2*b*t^2/US_PER_SECOND^2
#define timest2(b, t) ((timest(timest(b, (t)),(t)))/2)
// Macro to evaluate division of speed by acceleration (w/a), yielding time, in the appropriate units
#define wdiva(w, a) ((((w)*US_PER_MS)/a)*MS_PER_SECOND)

/**
 * Integer signal type with units of microseconds
 */
typedef int32_t ustime_t;

/**
 * Integer signal type with units of encoder counts
 */
typedef int32_t count_t;

/**
 * Integer signal type with units of encoder counts per second
 */
typedef int32_t rate_t;

/**
 * Integer signal type with units of encoder counts per second per second
 */
typedef int32_t accl_t;

/**
 * Integer signal type with units of duty (-10000, 10000)
 */
typedef int32_t duty_t;


/**
 * Motor control actions
 */
typedef enum {
    RUN,
    RUN_TIME,
    RUN_STALLED,
    RUN_ANGLE,
    RUN_TARGET,
    TRACK_TARGET,
} pbio_motor_action_t;

/**
 * Motor trajectory parameters for an ideal maneuver without disturbances
 */
typedef struct _pbio_motor_trajectory_t {
    pbio_motor_action_t action;         /**<  Motor action type */
    pbio_motor_after_stop_t after_stop; /**<  BRAKE, COAST or HOLD after maneuver */
    ustime_t t0;                        /**<  Time at start of maneuver */
    ustime_t t1;                        /**<  Time after the acceleration in-phase */
    ustime_t t2;                        /**<  Time at start of acceleration out-phase */
    ustime_t t3;                        /**<  Time at end of maneuver */
    count_t th0;                        /**<  Encoder count at start of maneuver */
    count_t th1;                        /**<  Encoder count after the acceleration in-phase */
    count_t th2;                        /**<  Encoder count at start of acceleration out-phase */
    count_t th3;                        /**<  Encoder count at end of maneuver */
    rate_t w0;                          /**<  Encoder rate at start of maneuver */
    rate_t w1;                          /**<  Encoder rate target when not accelerating */
    accl_t a0;                          /**<  Encoder acceleration during in-phase */
    accl_t a2;                          /**<  Encoder acceleration during out-phase */
} pbio_motor_trajectory_t;

pbio_motor_trajectory_t trajectories[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

void get_reference(ustime_t time_ref, pbio_motor_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref);

pbio_error_t make_motor_trajectory(pbio_port_t port,
                                   pbio_motor_action_t action,
                                   int32_t speed_target,
                                   int32_t duration_or_target_position,
                                   pbio_motor_after_stop_t after_stop);

#endif // _PBIO_MOTORREF_H_

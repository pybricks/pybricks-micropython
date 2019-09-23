// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_TRAJECTORY_H_
#define _PBIO_TRAJECTORY_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#define MS_PER_SECOND (1000)
#define US_PER_MS (1000)
#define US_PER_SECOND (1000000)

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
 * Motor trajectory parameters for an ideal maneuver without disturbances
 */
typedef struct _pbio_control_trajectory_t {
    bool forever;                       /**<  Whether maneuver has end-point */
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
} pbio_control_trajectory_t;

void make_trajectory_none(ustime_t t0, count_t th0, rate_t w1, pbio_control_trajectory_t *ref);

pbio_error_t make_trajectory_time_based(ustime_t t0, ustime_t t3, count_t th0, rate_t w0, rate_t wt, rate_t wmax, accl_t a, pbio_control_trajectory_t *ref);

pbio_error_t make_trajectory_time_based_forever(ustime_t t0, count_t th0, rate_t w0, rate_t wt, rate_t wmax, accl_t a, pbio_control_trajectory_t *ref);

pbio_error_t make_trajectory_angle_based(ustime_t t0, count_t th0, count_t th3, rate_t w0, rate_t wt, rate_t wmax, accl_t a, pbio_control_trajectory_t *ref);

void get_reference(ustime_t time_ref, pbio_control_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref);

#endif // _PBIO_TRAJECTORY_H_

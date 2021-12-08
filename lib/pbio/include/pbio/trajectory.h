// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_TRAJECTORY_H_
#define _PBIO_TRAJECTORY_H_

#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#define MS_PER_SECOND (1000)
#define US_PER_MS (1000)
#define US_PER_SECOND (1000000)

#define DURATION_FOREVER (-1)
#define DURATION_MAX_S (30 * 60)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// Macro to evaluate b*t/US_PER_SECOND in two steps to avoid excessive round-off errors and overflows.
#define timest(b, t) ((b * ((t) / US_PER_MS)) / MS_PER_SECOND)
// Same trick to evaluate formulas of the form 1/2*b*t^2/US_PER_SECOND^2
#define timest2(b, t) ((timest(timest(b, (t)),(t))) / 2)
// Macro to evaluate division of speed by acceleration (w/a), yielding time, in the appropriate units
#define wdiva(w, a) ((((w) * US_PER_MS) / a) * MS_PER_SECOND)

/**
 * Motor trajectory parameters for an ideal maneuver without disturbances
 */
typedef struct _pbio_trajectory_t {
    bool forever;                       /**<  Whether maneuver has end-point */
    int32_t t0;                        /**<  Time at start of maneuver */
    int32_t t1;                        /**<  Time after the acceleration in-phase */
    int32_t t2;                        /**<  Time at start of acceleration out-phase */
    int32_t t3;                        /**<  Time at end of maneuver */
    int32_t th0;                        /**<  Encoder count at start of maneuver */
    int32_t th1;                        /**<  Encoder count after the acceleration in-phase */
    int32_t th2;                        /**<  Encoder count at start of acceleration out-phase */
    int32_t th3;                        /**<  Encoder count at end of maneuver */
    int32_t th0_ext;                     /**<  As above, but additional millicounts */
    int32_t th1_ext;                     /**<  As above, but additional  millicounts */
    int32_t th2_ext;                     /**<  As above, but additional  millicounts */
    int32_t th3_ext;                     /**<  As above, but additional  millicounts */
    int32_t w0;                          /**<  Encoder rate at start of maneuver */
    int32_t w1;                          /**<  Encoder rate target when not accelerating */
    int32_t a0;                          /**<  Encoder acceleration during in-phase */
    int32_t a2;                          /**<  Encoder acceleration during out-phase */
} pbio_trajectory_t;

/**
 * Trajectory evaluated at a given point in time
 */
typedef struct _pbio_trajectory_reference_t {
    int32_t count;        /**<  Reference count */
    int32_t count_ext;    /**<  Reference count extra (sub-degree) */
    int32_t rate;         /**<  Reference rate */
    int32_t acceleration; /**<  Reference acceleration */
} pbio_trajectory_reference_t;

// Core trajectory generators

void pbio_trajectory_make_stationary(pbio_trajectory_t *trj, int32_t t0, int32_t th0);

// Make a trajectory with a fixed speed and final time, with arbitrary final angle.
pbio_error_t pbio_trajectory_calc_angle_new(pbio_trajectory_t *trj, int32_t t0, int32_t duration, int32_t th0, int32_t th0_ext, int32_t w0, int32_t wt, int32_t wmax, int32_t a);

// Make a trajectory with a fixed speed and final angle, with arbitrary final time
pbio_error_t pbio_trajectory_calc_time_new(pbio_trajectory_t *trj, int32_t t0, int32_t th0, int32_t th3, int32_t w0, int32_t wt, int32_t wmax, int32_t a);

// Stretches out a given trajectory time-wise to make it match time frames of other trajectory
void pbio_trajectory_stretch(pbio_trajectory_t *trj, int32_t t1mt0, int32_t t2mt0, int32_t t3mt0);

void pbio_trajectory_get_reference(pbio_trajectory_t *trj, int32_t time_ref, pbio_trajectory_reference_t *ref);

// Extended and patched trajectories

pbio_error_t pbio_trajectory_calc_angle_extend(pbio_trajectory_t *trj, int32_t t0, int32_t t3, int32_t wt, int32_t wmax, int32_t a);

pbio_error_t pbio_trajectory_calc_time_extend(pbio_trajectory_t *trj, int32_t t0, int32_t th3, int32_t wt, int32_t wmax, int32_t a);


#endif // _PBIO_TRAJECTORY_H_

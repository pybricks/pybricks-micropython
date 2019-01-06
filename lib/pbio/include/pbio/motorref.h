/*
 * Copyright (c) 2018 Laurens Valk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PBIO_MOTORREF_H_
#define _PBIO_MOTORREF_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

#define NONE (0) // A "don't care" constant for readibility of the code, but which is never used after assignment
#define NONZERO (100) // Arbitrary nonzero speed
#define max_abs_accl (1000000) // "Infinite" acceleration, equivalent to reaching 1000 deg/s in just 1 milisecond.

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
 * NEW Motor trajectory parameters
 */
typedef struct _pbio_motor_ref_t {
    ustime_t t0, t1, t2, t3;
    count_t th0, th1, th2, th3;
    rate_t w0, w1;
    accl_t a0, a2;
} pbio_motor_ref_t;

pbio_motor_ref_t refs[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];


/**
 * OLD, will be phased out. Motor trajectory parameters for an ideal maneuver without disturbances
 */
typedef struct _pbio_motor_trajectory_t {
    pbio_motor_action_t action;         /**<  Motor action type */
    pbio_motor_after_stop_t after_stop; /**<  BRAKE, COAST or HOLD after maneuver */
    ustime_t time_start;                /**<  Time at start of maneuver */
    ustime_t time_in;                   /**<  Time after the acceleration in-phase */
    ustime_t time_out;                  /**<  Time at start of acceleration out-phase */
    ustime_t time_end;                  /**<  Time at end of maneuver */
    count_t count_start;                /**<  Encoder count at start of maneuver */
    count_t count_in;                   /**<  Encoder count after the acceleration in-phase */
    count_t count_out;                  /**<  Encoder count at start of acceleration out-phase */
    count_t count_end;                  /**<  Encoder count at end of maneuver */
    rate_t rate_start;                  /**<  Encoder rate at start of maneuver */
    rate_t rate_target;                 /**<  Encoder rate target when not accelerating */
    accl_t accl_start;                  /**<  Encoder acceleration during in-phase */
    accl_t accl_end;                    /**<  Encoder acceleration during out-phase */
} pbio_motor_trajectory_t;

pbio_motor_trajectory_t trajectories[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

void get_reference(ustime_t time_ref, pbio_motor_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref);

pbio_error_t make_motor_trajectory(pbio_port_t port,
                                   pbio_motor_action_t action,
                                   int32_t speed_target,
                                   int32_t duration_or_target_position,
                                   pbio_motor_after_stop_t after_stop);

#endif // _PBIO_MOTORREF_H_

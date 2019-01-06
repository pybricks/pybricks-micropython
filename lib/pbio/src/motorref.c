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

#include <pbio/motorref.h>
#include <stdlib.h>
#include <math.h>

#include "sys/clock.h"

void reverse_trajectory(pbio_motor_ref_t *ref){
    // Mirror angles about initial angle th0
    ref->th1 = 2*ref->th0 - ref->th1;
    ref->th2 = 2*ref->th0 - ref->th2;
    ref->th3 = 2*ref->th0 - ref->th3;

    // Negate speeds and accelerations
    ref->w0 *= -1;
    ref->w1 *= -1;
    ref->a0 *= -1;
    ref->a2 *= -1;
}

void make_trajectory_none(ustime_t t0, count_t th0, pbio_motor_ref_t *ref){
    // All times equal to initial time:
    ref->t0 = t0;
    ref->t1 = t0;
    ref->t2 = t0;
    ref->t3 = t0;

    // All angles equal to initial angle:
    ref->th0 = th0;
    ref->th1 = th0;
    ref->th2 = th0;
    ref->th3 = th0;

    // All speeds/accelerations zero:
    ref->w0 = 0;
    ref->w1 = 0;
    ref->a0 = 0;
    ref->a2 = 0;   
}

pbio_error_t make_trajectory_time_based(ustime_t t0, ustime_t t3, count_t th0, rate_t w0, rate_t wt, rate_t wmax, accl_t a, pbio_motor_ref_t *ref){

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    ustime_t t3mt0 = t3-t0;
    ustime_t t3mt2;
    ustime_t t2mt1;
    ustime_t t1mt0;

    // Return error for negative user-specified duration
    if (t3mt0 < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = wt < 0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        wt *= -1;
        w0 *= -1;
    }

    // Limit initial speed
    rate_t max_init = timest(a, t3mt0);
    rate_t abs_max = min(wmax, max_init);
    w0 = max(-abs_max, min(w0, abs_max));

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate
        ref->a0 = a;
        // If target speed can be reached
        if (wdiva(wt-w0, a) - (t3mt0-wdiva(w0, a))/2 < 0) {
            t1mt0 = wdiva(wt-w0, a);
            ref->w1 = wt;
        }
        // If target speed cannot be reached
        else {
            t1mt0 = (t3mt0-wdiva(w0, a))/2;
            ref->w1 = timest(a, t3mt0)/2+w0/2;
        }
    }
    // Initial speed is equal to or more than the target speed
    else {
        // Therefore decelerate
        ref->a0 = -a;
        t1mt0 = wdiva(w0-wt, a);
        ref->w1 = wt;
    }

    // # Deceleration phase
    ref->a2 = -a;
    t3mt2 =  wdiva(ref->w1, a);
    
    // Constant speed duration
    t2mt1 = t3mt0 - t3mt2 - t1mt0;

    // Assert that all time intervals are positive
    if (t1mt0 < 0 || t2mt1 < 0 || t3mt2 < 0) {
        return PBIO_ERROR_FAILED;
    }

    // Store other results/arguments
    ref->w0 = w0;
    ref->th0 = th0;
    ref->t0 = t0;
    ref->t1 = t0 + t1mt0;
    ref->t2 = t0 + t1mt0 + t2mt1;
    ref->t3 = t3;

    // Corresponding angle values
    ref->th1 = ref->th0 + timest(ref->w0, t1mt0) + timest2(ref->a0, t1mt0);
    ref->th2 = ref->th1 + timest(ref->w1, t2mt1);
    ref->th3 = ref->th2 + timest(ref->w1, t3mt2) + timest2(ref->a2, t3mt2);

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(ref);
    }

    return PBIO_SUCCESS;
}


pbio_error_t make_trajectory_angle_based(ustime_t t0, count_t th0, count_t th3, rate_t w0, rate_t wt, rate_t wmax, accl_t a, pbio_motor_ref_t *ref){

    // Return error for zero speed
    if (wt == 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Return empty maneuver for zero angle
    if (th3 == th0) {
        make_trajectory_none(t0, th0, ref);
        return PBIO_SUCCESS;
    }    

    // Remember if the original user-specified maneuver was backward
    bool backward = th3 < th0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        th3 = 2*th0 - th3;
        w0 *= -1;
    }
    wt = abs(wt);

    // Limit initial speed, but evaluate square root only if necessary (usually not)
    if (w0 > 0 && (w0*w0)/(2*a) > th3 - th0) {
        w0 = sqrt(2*a*(th3 - th0)); // TODO: Use int implementation
    }

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate towards intersection from below,
        // either by reaching constant speed phase or not.
        ref->a0 = a;

        // Fictitious zero speed angle (ahead of us if we have negative initial speed; behind us if we have initial positive speed)
        count_t thf = th0 - (w0*w0)/(2*a);

        // Test if we can get to ref speed
        if (th3-thf >= (wt*wt)/a) {
            //  If so, find both constant speed intersections
            ref->th1 = thf + (wt*wt)/(2*a);
            ref->th2 = th3 - (wt*wt)/(2*a);
            ref->w1 = wt;
        }
        else {
            // Otherwise, intersect halfway between accelerating and decelerating square root arcs
            ref->th1 = (th3+thf)/2;
            ref->th2 = ref->th1;
            ref->w1 = sqrt(2*a*(ref->th1-thf)); // TODO: Use int implementation
        }
    }
    // Initial speed is equal to or more than the target speed
    else {
        // Therefore decelerate towards intersection from above
        ref->a0 = -a;
        ref->th1 = th0 + (w0*w0-wt*wt)/(2*a);
        ref->th2 = th3 - (wt*wt)/(2*a);
        ref->w1 = wt;
    }

    // Corresponding time intervals
    ustime_t t1mt0 = wdiva(ref->w1-w0, ref->a0);
    ustime_t t2mt1 = wdiva(ref->th2-ref->th1, ref->w1);
    ustime_t t3mt2 = wdiva(ref->w1, a);

    // Store other results/arguments
    ref->w0 = w0;
    ref->th0 = th0;
    ref->th3 = th3;
    ref->t0 = t0;
    ref->t1 = t0 + t1mt0;
    ref->t2 = ref->t1 + t2mt1;
    ref->t3 = ref->t2 + t3mt2;
    ref->a2 = -a;

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(ref);
    }

    return PBIO_SUCCESS;
}

/////////////////////OLD////////////////////////////

// Return max(-limit, min(value, limit)): Limit the magnitude of value to be equal to or less than provided limit
float_t limit(float_t value, float_t limit){
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

// Return 'value' with the sign of 'signof'. Equivalent to: sgn(signof)*abs(value)
float_t signval(float_t signof, float_t value) {
    if (signof > 0) {
        return abs(value);
    }
    if (signof < 0){
        return -abs(value);
    }
    return 0;
}

// Calculate the characteristic time values, encoder values, rate values and accelerations that uniquely define the rate and count trajectories
pbio_error_t make_motor_trajectory(pbio_port_t port,
                                   pbio_motor_action_t action,
                                   int32_t speed_target,
                                   int32_t duration_or_target_position,
                                   pbio_motor_after_stop_t after_stop){

    // Read the current system state for this motor
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err = pbio_encmotor_get_encoder_count(port, &count_start);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    pbio_encmotor_get_encoder_rate(port, &rate_start);
    if (err != PBIO_SUCCESS) {
        return err;
    }    
    pbio_motor_trajectory_t *traject = &trajectories[PORT_TO_IDX(port)];
    pbio_encmotor_settings_t *settings = &encmotor_settings[PORT_TO_IDX(port)];


    // Work with floats for now (only in this function, which is called only once when a maneuver is called.)
    float_t _time_start = ((float_t) time_start)/US_PER_SECOND;
    float_t _time_in = 0;
    float_t _time_out = 0;
    float_t _time_end = 0;
    float_t _count_start = count_start;
    float_t _count_in = 0;
    float_t _count_out = 0;
    float_t _count_end = 0;
    float_t _rate_start = rate_start;
    float_t _rate_target = speed_target * settings->counts_per_output_unit;
    float_t _accl_start = 0;
    float_t _accl_end = 0;

    // Limit reference rates
    _rate_start = limit(_rate_start, settings->max_rate);
    _rate_target = limit(_rate_target, settings->max_rate);

    // Set endpoint (time or angle), depending on selected action
    switch(action){
        case RUN:
            // FOR RUN and RUN_STALLED, we specify no end time
            _time_end = NONE;
            break;
        case RUN_TIME:
            // Do not allow negative time
            if (duration_or_target_position < 0) {
                return PBIO_ERROR_INVALID_ARG;
            }
            // For RUN_TIME, the end time is the current time plus the duration
            _time_end = _time_start + ((float_t) duration_or_target_position)/MS_PER_SECOND;
            break;
        case RUN_STALLED:
            // FOR RUN and RUN_STALLED, we specify no end time
            _time_end = NONE;
            break;
        case RUN_ANGLE:
            // For RUN_ANGLE, we specify instead the end count value as the current value plus the requested angle
            _count_end = _count_start + duration_or_target_position * settings->counts_per_output_unit;
            // If the goal is to reach a relative target, the speed cannot not be zero
            if (speed_target == 0) {
                return PBIO_ERROR_INVALID_ARG;
            }
            break;
        case RUN_TARGET:
            // For RUN_TARGET, we specify instead the end count value
            _count_end = duration_or_target_position * settings->counts_per_output_unit;
            // If the goal is to reach a position target, the speed cannot not be zero
            if (speed_target == 0) {
                return PBIO_ERROR_INVALID_ARG;
            }
            break;
        case TRACK_TARGET:
            // TODO
            break;
    }

    // Return an empty maneuver if ...
    if (
        // ... the specified final angle is equal to the corresponding starting value
        ((action == RUN_TARGET  || action == RUN_ANGLE) && ((count_t) _count_end) == count_start) ||
        // ... or the end time is equal to the specified end time
        (action == RUN_TIME  && ((ustime_t) (_time_end*US_PER_SECOND)) <= time_start) ||
        // ... or the initial speed equals the target speed of a run command
        (action == RUN && rate_start == speed_target)
       )
    {
        traject->time_start = time_start;
        traject->time_in = time_start;
        traject->time_out = time_start;
        traject->time_end = time_start;
        traject->count_start = count_start;
        traject->count_in = count_start;
        traject->count_out = count_start;
        traject->count_end = count_start;
        traject->rate_start = 0;
        traject->rate_target = 0;
        traject->accl_start = 0;
        traject->accl_end = 0;
        traject->action = action;
        traject->after_stop = after_stop;
        return PBIO_SUCCESS;
    }

    // Determine sign of reference rate in case of position target. The rate sign specified by the user is ignored
    if (action == RUN_TARGET || action == RUN_ANGLE) {
        // If the target is ahead of us, go forward. Otherwise go backward.
        _rate_target = (_count_end > _count_start) ? abs(_rate_target) : -abs(_rate_target);
    }
    // For time based control, the direction is taken into account as well
    else if (action == RUN || action == RUN_TIME || action == RUN_STALLED) {
        _rate_target = _rate_target;
    }
    // To reduce complexity for now, we assume that the direction does not change during the acceleration phase.
    // If a reversal is requested, this therefore means an immediate reveral, and then a smooth acceleration to the
    // desired rate. This can be improved in future versions.
    if ((_rate_target < 0 && _rate_start > 0) || (_rate_target > 0 && _rate_start < 0)){
        _rate_start = 0;
    }

    // Accelerations with sign
    _accl_start = (_rate_target > _rate_start) ? settings->abs_accl_start : -settings->abs_accl_start;
    _accl_end = _rate_target > 0 ? -settings->abs_accl_end : settings->abs_accl_end;

    // Limit reference speeds if move is shorter than full in/out phase (time_based case)
    if (action == RUN_TIME && _time_end - _time_start < (_rate_target-_rate_start)/_accl_start - _rate_target/_accl_end) {
        // If we are here, there is not enough time to fully accelerate and decelerate as desired.
        // If the initial rate is less than the target rate, we can reduce the target rate to account for this.
        if (abs(_rate_start) < abs(_rate_target)) {
            _rate_target = _accl_end*_accl_start/(_accl_end-_accl_start)*(_time_end-_time_start + _rate_start/_accl_start);
        }
        // Otherwise, disable the initial acceleration phase, and check if this gives enough time to decelerate
        else {
            // Set to maximum initial acceleration
            _accl_start = signval(_accl_start, settings->abs_accl_start);

            // If there is not even enough time for just the out-phase, reduce that phase too.
            if ( _time_end - _time_start < -_rate_target/_accl_end) {
                // Limit the target speed such that if we decellerate at the desired rate, we reach zero speed at the end time.
                _rate_target = -_accl_end*(_time_end-_time_start);
            }
            // Limit the start rate by the reduced target rate
            _rate_start = _rate_target;
        }

    }

    // Limit reference speeds if move is shorter than full in/out phase (RUN_TARGET || RUN_ANGLE case)
    if ((action == RUN_TARGET || action == RUN_ANGLE) && abs(_count_end-_count_start) < abs((_rate_target*_rate_target-_rate_start*_rate_start)/(2*_accl_start)) + abs(_rate_target*_rate_target/(2*_accl_end))) {
        // There is not enough angle for the in and out phase
        if (abs(_rate_start) < abs(_rate_target)) {
            // Limit _rate_target to make in-and-out intersect because _rate_start is low enough
            _rate_target = signval(_rate_target, sqrtf(abs(_accl_start*_accl_end/(_accl_end-_accl_start)*(2*_count_end-2*_count_start+_rate_start*_rate_start/_accl_start))));
        }
        else {
            // Let us disable the in-phase, and check if there is sufficient angle for out-phase
            _accl_start = signval(_accl_start, max_abs_accl);
            if (abs(_count_end-_count_start) < abs(_rate_target*_rate_target/_accl_end/2)) {
                // Limit _rate_target as well to at least make the out-phase feasible
                _rate_target = signval(_rate_target, sqrtf(2*_accl_end*(_count_start-_count_end)));
            }
            // Limit the start rate by the reduced target rate
            _rate_start = _rate_target;
        }
    }

    // Compute intermediate time and angle values just after initial acceleration, except for the stop action, which has no initial phase
    _time_in = _time_start + (_rate_target-_rate_start)/_accl_start;
    _count_in = _count_start + ((_rate_target*_rate_target)-(_rate_start*_rate_start))/_accl_start/2;

    // Compute intermediate time and angle values just before deceleration and end time or end angle, depending on which is already given
    if (action == RUN_TIME) {
        _time_out = _time_end + _rate_target/_accl_end;
        _count_out = _count_in + _rate_target*(_time_out-_time_in);
        _count_end = _count_out - _rate_target*_rate_target/_accl_end/2;
    }
    else if (action == RUN_TARGET || action == RUN_ANGLE) {
        _time_out = _time_in + (_count_end-_count_in)/_rate_target + _rate_target/_accl_end/2;
        _count_out = _count_in + _rate_target*(_time_out-_time_in);
        _time_end = _time_out - _rate_target/_accl_end;
    }
    else {
        _time_out = NONE;
        _time_end = NONE;
        _count_out = NONE;
        _count_end = NONE;
    }

    // Convert temporary float results back to integers
    traject->time_start = _time_start * US_PER_SECOND;
    traject->time_in = _time_in * US_PER_SECOND;
    traject->time_out = _time_out * US_PER_SECOND;
    traject->time_end = _time_end * US_PER_SECOND;
    traject->count_start = _count_start;
    traject->count_in = _count_in;
    traject->count_out = _count_out;
    traject->count_end = _count_end;
    traject->rate_start = _rate_start;
    traject->rate_target = _rate_target;
    traject->accl_start = _accl_start;
    traject->accl_end = _accl_end;
    traject->action = action;
    traject->after_stop = after_stop;

    if (action == RUN_TIME || action == RUN_TARGET || action == RUN_ANGLE) {
        // PRINT ORIGINAL MANEUVER
        printf("\nPort       : %c\nAction     : %d\nAfter stop : %d\ntime_start : %u\ntime_in    : %u\ntime_out   : %u\ntime_end   : %u\ncount_start: %d\ncount_in   : %d\ncount_out  : %d\ncount_end  : %d\nrate_start : %d\nrate_target: %d\naccl_start : %d\naccl_end   : %d\n", 
            port,
            (int)traject->action,
            (int)traject->after_stop,
            (int)traject->time_start,
            (int)(traject->time_in-traject->time_start),
            (int)(traject->time_out-traject->time_start),
            (int)(traject->time_end-traject->time_start),
            (int)traject->count_start,
            (int)traject->count_in,
            (int)traject->count_out,
            (int)traject->count_end,
            (int)traject->rate_start,
            (int)traject->rate_target,
            (int)traject->accl_start,
            (int)traject->accl_end
        );
        // PRINT NEW MANEUVER    
        pbio_motor_ref_t *test = &refs[PORT_TO_IDX(port)];
        if (action == RUN_TARGET) {
            make_trajectory_angle_based(time_start, count_start, duration_or_target_position, rate_start, speed_target, settings->max_rate, settings->abs_accl_start, test);
        }   
        else if (action == RUN_ANGLE) {
            make_trajectory_angle_based(time_start, count_start, count_start + duration_or_target_position, rate_start, speed_target, settings->max_rate, settings->abs_accl_start, test);
        }
        else { // RUN_TIME
            make_trajectory_time_based(time_start, time_start + duration_or_target_position*US_PER_MS, count_start, rate_start, speed_target, settings->max_rate, settings->abs_accl_start, test);
        }
        
        printf(
            "-----------------\n"
            "t0   : %d\n"
            "t1-t0: %d\n"
            "t2-t0: %d\n"
            "t3-t0: %d\n"
            "th0  : %d\n"
            "th1  : %d\n"
            "th2  : %d\n"
            "th3  : %d\n"
            "w0   : %d\n"
            "w1   : %d\n"
            "a0   : %d\n"
            "a2   : %d\n", 
            test->t0,
            test->t1-test->t0,
            test->t2-test->t0,
            test->t3-test->t0,
            test->th0,
            test->th1,
            test->th2,
            test->th3,
            test->w0,
            test->w1,
            test->a0,
            test->a2
        );     
    }

    return PBIO_SUCCESS;
}

// Evaluate the reference speed and velocity at the (shifted) time
void get_reference(ustime_t time_ref, pbio_motor_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref){
    // For RUN and RUN_STALLED, the end time is infinite, meaning that the reference signals do not have a deceleration phase
    if (time_ref - traject->time_in < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        *rate_ref = traject->rate_start   + timest(traject->accl_start, time_ref-traject->time_start);
        *count_ref = traject->count_start + timest(traject->rate_start, time_ref-traject->time_start) + timest2(traject->accl_start, time_ref-traject->time_start);
    }
    else if ((traject->action == RUN) || (traject->action == RUN_STALLED) || time_ref - traject->time_out <= 0) {
        // If we are here, then we are in the constant speed phase
        *rate_ref = traject->rate_target;
        *count_ref = traject->count_in + timest(traject->rate_target, time_ref-traject->time_in);
    }
    else if (time_ref - traject->time_end <= 0) {
        // If we are here, then we are in the deceleration phase
        *rate_ref = traject->rate_target + timest(traject->accl_end,    time_ref-traject->time_out);
        *count_ref = traject->count_out  + timest(traject->rate_target, time_ref-traject->time_out) + timest2(traject->accl_end, time_ref-traject->time_out);
    }
    else {
        // If we are here, we are in the zero speed phase (relevant when holding position)
        *rate_ref = 0;
        *count_ref = traject->count_end;
    }
}

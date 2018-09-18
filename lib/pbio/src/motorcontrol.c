#include <pbio/motorcontrol.h>
#include <stdatomic.h>
#include <pbdrv/time.h>
#include <stdlib.h>
#include <math.h>

// A "don't care" constant for readibility of the code, but which is never used after assignment
// Typically used for parameters that have no effect for the selected maneuvers.
#define NONE (0)
#define max_abs_accl (1000000)

// Units and prescalers to enable integer divisions
#define NUM_SCALE (10000)
#define DEN_SCALE (US_PER_SECOND / NUM_SCALE)

/**
 * Unsigned integer type with units of microseconds
 */
typedef uint32_t ustime_t;

/**
 * Integer type with units of encoder counts
 */
typedef int32_t count_t;

/**
 * Integer type with units of encoder counts per second
 */
typedef int32_t rate_t;

/**
 * Integer type with units of encoder counts per second per second
 */
typedef int32_t accl_t;

/**
 * Motor control actions
 */
typedef enum {
    IDLE,
    RUN,
    STOP,
    RUN_TIME,
    RUN_STALLED,
    RUN_ANGLE,
    RUN_TARGET,
    TRACK_TARGET,
} pbio_motor_action_t;


// Atomic flag, one for each motor, that is set when the command_new variable is currently being read or being written. It is clear when it is free.
volatile atomic_flag busy[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        false
    }
};


/**
 * Motor trajectory parameters for an ideal maneuver without disturbances
 */
typedef struct _pbio_motor_trajectory_t {
    pbio_motor_action_t action; // Motor action type
    pbio_motor_after_stop_t after_stop; // BRAKE, COAST or HOLD after maneuver
    pbio_motor_wait_t wait;     // Wait for maneuver to complete
    ustime_t time_start;        // Time at start of maneuver
    ustime_t time_in;           // Time after the acceleration in-phase
    ustime_t time_out;          // Time at start of acceleration out-phase
    ustime_t time_end;          // Time at end of maneuver
    count_t count_start;        // Encoder count at start of maneuver
    count_t count_in;           // Encoder count after the acceleration in-phase
    count_t count_out;          // Encoder count at start of acceleration out-phase
    count_t count_end;          // Encoder count at end of maneuver
    rate_t rate_start;          // Encoder rate at start of maneuver
    rate_t rate_target;         // Encoder rate target when not accelerating
    accl_t accl_start;          // Encoder acceleration during in-phase
    accl_t accl_end;            // Encoder acceleration during out-phase
} pbio_motor_trajectory_t;

// Initialize current command to idle
pbio_motor_trajectory_t trajectories[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .time_start = 0,
        .time_in = 0,
        .time_out = 0,
        .time_end = 0,
        .count_start = 0,
        .count_in = 0,
        .count_out = 0,
        .count_end = 0,
        .rate_start = 0,
        .rate_target = 0,
        .accl_start = 0,
        .accl_end = 0
    }
};

pbio_error_t make_motor_command(pbio_port_t port,
                                pbio_motor_action_t action,
                                float_t rate_target,
                                float_t duration_or_target_count,
                                pbio_motor_after_stop_t after_stop,
                                pbio_motor_wait_t wait);

pbio_error_t pbio_encmotor_run(pbio_port_t port, float_t speed){
    return make_motor_command(port, RUN, speed, NONE, NONE, NONE);
}

pbio_error_t pbio_encmotor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return PBIO_ERROR_FAILED; // Not yet implemented in theory
}

pbio_error_t pbio_encmotor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return make_motor_command(port, RUN_TIME, speed, duration, after_stop, wait);
}

pbio_error_t pbio_encmotor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return make_motor_command(port, RUN_STALLED, speed, NONE, after_stop, wait);
    // Implement conditional waits... + conditional return
}

pbio_error_t pbio_encmotor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return make_motor_command(port, RUN_ANGLE, speed, angle, after_stop, wait);
}

pbio_error_t pbio_encmotor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return make_motor_command(port, RUN_TARGET, speed, target, after_stop, wait);
}

pbio_error_t pbio_encmotor_track_target(pbio_port_t port, float_t target){
    return PBIO_ERROR_FAILED; // Not yet implemented in theory
}

void debug_trajectory(pbio_port_t port){
    pbio_motor_trajectory_t *traject = &trajectories[PORT_TO_IDX(port)];
    printf("\nPort       : %c\nAction     : %d\nAfter stop : %d\nWait       : %d\ntime_start : %u\ntime_in    : %u\ntime_out   : %u\ntime_end   : %u\ncount_start: %d\ncount_in   : %d\ncount_out  : %d\ncount_end  : %d\nrate_start : %d\nrate_target: %d\naccl_start : %d\naccl_end   : %d\n", 
        port,
        (int)traject->action,
        (int)traject->after_stop,
        (int)traject->wait,
        (unsigned int)traject->time_start,
        (unsigned int)(traject->time_in-traject->time_start),
        (unsigned int)(traject->time_out-traject->time_start),
        (unsigned int)(traject->time_end-traject->time_start),
        (int)traject->count_start,
        (int)traject->count_in,
        (int)traject->count_out,
        (int)traject->count_end,
        (int)traject->rate_start,
        (int)traject->rate_target,
        (int)traject->accl_start,
        (int)traject->accl_end
    );
}

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
pbio_error_t make_motor_command(pbio_port_t port,
                                pbio_motor_action_t action,
                                float_t rate_target,
                                float_t duration_or_target_count,
                                pbio_motor_after_stop_t after_stop,
                                pbio_motor_wait_t wait){    

    // Read the current system state for this motor
    ustime_t time_start = pbdrv_get_time_usec();
    count_t count_start;
    rate_t rate_start;
    pbio_encmotor_get_encoder_count(port, &count_start);
    pbio_encmotor_get_encoder_rate(port, &rate_start);
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
    float_t _rate_target = rate_target * settings->counts_per_output_unit;
    float_t _accl_start = 0;
    float_t _accl_end = 0;

    // Set endpoint (time or angle), depending on selected action
    switch(action){
        case IDLE:
            // TODO
            break;
        case STOP:
            // TODO
            break;            
        case RUN:
            // FOR RUN and RUN_STALLED, we specify no end time
            _time_end = NONE;        
            break;
        case RUN_TIME:
            // Do not allow negative time
            if (duration_or_target_count < 0) {
                return PBIO_ERROR_INVALID_ARG;
            }
            // For RUN_TIME, the end time is the current time plus the duration
            _time_end = _time_start + duration_or_target_count / US_PER_SECOND;
            break;
        case RUN_STALLED:
            // FOR RUN and RUN_STALLED, we specify no end time
            _time_end = NONE;        
            break;
        case RUN_ANGLE:
            // For RUN_ANGLE, we specify instead the end count value as the current value plus the requested angle
            _count_end = _count_start + duration_or_target_count * settings->counts_per_output_unit;
            // If the goal is to reach a relative target, the speed cannot not be zero
            if (rate_target == 0) {
                return PBIO_ERROR_INVALID_ARG;
            }
            break;
        case RUN_TARGET:
            // For RUN_TARGET, we specify instead the end count value
            _count_end = duration_or_target_count * settings->counts_per_output_unit;
            // If the goal is to reach a position target, the speed cannot not be zero
            if (rate_target == 0) {
                return PBIO_ERROR_INVALID_ARG;
            }        
            break;
        case TRACK_TARGET:
            // TODO
            break;
    }

    // If the specified endpoint (angle or finite time) is equal to the corresponding starting value, return an empty maneuver.
    if ( ((action == RUN_TARGET  || action == RUN_ANGLE) && ((count_t) _count_end) == count_start) ||
          (action == RUN_TIME  && ((ustime_t) (_time_end*US_PER_SECOND)) <= time_start)) {
        while(atomic_flag_test_and_set(&busy[PORT_TO_IDX(port)])); // Remove once we remove multithreading
        traject->time_in = time_start;
        traject->time_out = time_start;
        traject->time_end = time_start;
        traject->count_in = count_start;
        traject->count_out = count_start;
        traject->count_end = count_start;
        traject->rate_start = 0;
        traject->rate_target = 0;
        traject->accl_start = 0;
        traject->accl_end = 0;
        traject->action = action;
        traject->after_stop = after_stop;
        traject->wait = wait;
        atomic_flag_clear(&busy[PORT_TO_IDX(port)]);  // Remove once we remove multithreading
        return PBIO_SUCCESS;
    }

    // Limit reference rates
    _rate_start = limit(_rate_start, settings->max_rate);
    _rate_target = limit(_rate_target, settings->max_rate);

    // Determine sign of reference rate in case of position target. The rate sign specified by the user is ignored
    if (action == RUN_TARGET) {
        // If the target is ahead of us, go forward. Otherwise go backward.
        _rate_target = (_count_end > _count_start) ? abs(_rate_target) : -abs(_rate_target);
    }

    // For time based control, the direction is taken into account as well
    if (action == RUN || action == RUN_TIME || action == RUN_STALLED) {
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

    // Limit reference speeds if move is shorter than full in/out phase (RUN_TARGET case)
    if (action == RUN_TARGET && abs(_count_end-_count_start) < abs((_rate_target*_rate_target-_rate_start*_rate_start)/(2*_accl_start)) + abs(_rate_target*_rate_target/(2*_accl_end))) {
        // There is not enough angle for the in and out phase
        if (abs(_rate_start) < abs(_rate_target)) {
            // Limit _rate_target to make in-and-out intersect because _rate_start is low enough
            _rate_target = signval(_rate_target, sqrt(abs(_accl_start*_accl_end/(_accl_end-_accl_start)*(2*_count_end-2*_count_start+_rate_start*_rate_start/_accl_start))));
        }
        else {
            // Let us disable the in-phase, and check if there is sufficient angle for out-phase
            _accl_start = signval(_accl_start, max_abs_accl);
            if (abs(_count_end-_count_start) < abs(_rate_target*_rate_target/_accl_end/2)) {
                // Limit _rate_target as well to at least make the out-phase feasible
                _rate_target = signval(_rate_target, sqrt(2*_accl_end*(_count_start-_count_end)));
            }
            // Limit the start rate by the reduced target rate
            _rate_start = _rate_target;
        }   
    }

    // Compute intermediate time and angle values just after initial acceleration
    _time_in = _time_start + (_rate_target-_rate_start)/_accl_start;
    _count_in = _count_start + ((_rate_target*_rate_target)-(_rate_start*_rate_start))/_accl_start/2;

    // Compute intermediate time and angle values just before deceleration and end time or end angle, depending on which is already given
    if (action == RUN_TIME) {
        _time_out = _time_end + _rate_target/_accl_end;
        _count_out = _count_in + _rate_target*(_time_out-_time_in);
        _count_end = _count_out - _rate_target*_rate_target/_accl_end/2;
    }
    else if (action == RUN_TARGET) {
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
    while(atomic_flag_test_and_set(&busy[PORT_TO_IDX(port)])); // Remove once we remove multithreading
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
    traject->wait = wait;
    atomic_flag_clear(&busy[PORT_TO_IDX(port)]);  // Remove once we remove multithreading
    return PBIO_SUCCESS;
}

// Todo: init with actual time
ustime_t time_started[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)] = 0
};

//TODO: Translate python snippets below, including workarounds for integers

// Evaluate the reference speed and velocity at the (shifted) time
void get_reference(ustime_t time_ref, pbio_motor_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref){

    // For RUN and RUN_STALLED, the end time is infinite, meaning that the reference signals do not have a deceleration phase
    bool infinite = (traject->action == RUN) || (traject->action == RUN_STALLED);

    if (time_ref < traject->time_in) {
        // If we are here, then we are still in the acceleration phase
        //         omega_ref = omega_0 + alpha_in*(time_ref-time_0)
        //         theta_ref = theta_0 + omega_0*(time_ref-time_0) + alpha_in/2*(time_ref-time_0)**2
    }
    else if (!infinite && time_ref <= traject->time_out) {
        // If we are here, then we are in the constant speed phase
        //         omega_ref = omega_star
        //         theta_ref = theta_in + omega_star*(time_ref-time_in)
    }
    else if (!infinite && time_ref <= traject->time_end) {
        // If we are here, then we are in the deceleration phase
        //         omega_ref = omega_star + alpha_out*(time_ref-time_out)
        //         theta_ref = theta_out + omega_star*(time_ref-time_out) + alpha_out/2*(time_ref-time_out)**2        
    }
    else {
        // If we are here, we are in the zero speed phase (relevant when holding position)
        //         omega_ref = 0
        //         theta_ref = theta_end  
    } 
}

void motor_control_update(){

    ustime_t time_now;
    count_t count_now, count_ref;
    rate_t rate_now, rate_ref; 

    // Do the update for each motor
    for (pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++){
        // Port index
        uint8_t idx = PORT_TO_IDX(port);

        // If we have read access, process
        if (!atomic_flag_test_and_set(&busy[idx])) { // Remove once we remove multithreading
            
            if (trajectories[idx].time_start != time_started[idx]){
                // If we are here, then we have to start a new command  
                time_started[idx] = trajectories[idx].time_start;
                debug_trajectory(port);
            }
            atomic_flag_clear(&busy[idx]); // Remove once we remove multithreading
        }
        // Read current state of this motor: current time, speed, and position
        time_now = pbdrv_get_time_usec();
        pbio_encmotor_get_encoder_count(port, &count_now);
        pbio_encmotor_get_encoder_rate(port, &rate_now);   
        get_reference(time_now, &trajectories[idx], &count_ref, &rate_ref);

        // Calculate control signal for current state and current command
        
        // Set the duty cycle
    }
}

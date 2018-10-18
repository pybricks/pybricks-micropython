#include <pbio/motorcontrol.h>
#include <pbdrv/time.h>
#include <stdlib.h>
#include <math.h>

#define NONE (0) // A "don't care" constant for readibility of the code, but which is never used after assignment
#define NONZERO (100) // Arbitrary nonzero speed
#define max_abs_accl (1000000) // "Infinite" acceleration, equivalent to reaching 1000 deg/s in just 1 milisecond.

// Macro to evaluate b*t/US_PER_SECOND in two steps to avoid excessive round-off errors and overflows.
#define timest(b, t) ((b * ((t)/US_PER_MS))/MS_PER_SECOND)
// Same trick to evaluate formulas of the form 1/2*b*t^2/US_PER_SECOND^2
#define timest2(b, t) ((timest(timest(b, (t)),(t)))/2)

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
    STOP,
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

/**
 * Status of the anti-windup integrators
 */
typedef enum {
    /**< Initial status prior to initialization */
    TIME_NOT_INITIALIZED,
    /**< Anti-windup status for PID position control:
         Pause the position and speed trajectory when
         the motor is stalled by pausing time. */ 
    TIME_PAUSED,
    TIME_RUNNING,
    /**< Anti-windup status for PI speed control:
         Pause the integration of the
         accumulated speed error when stalled. */
    SPEED_INTEGRATOR_RUNNING,
    SPEED_INTEGRATOR_PAUSED,
} windup_status_t;

typedef enum {
    /**< Motor is not stalled */
    STALLED_NONE = 0x00,
    /**< The proportional duty control term is larger than the maximum and still the motor moves slower than specified limit */
    STALLED_PROPORTIONAL = 0x01,
    /**< The integral duty control term is larger than the maximum and still the motor moves slower than specified limit */
    STALLED_INTEGRAL = 0x02,
} stalled_status_t;

/**
 * Motor PID control status
 */
typedef struct _pbio_motor_control_status_t {
    windup_status_t windup_status; /**< State of the anti-windup variables */
    stalled_status_t stalled;      /**< Stalled state of the motor */
    count_t err_integral;          /**< Integral of position error (RUN_ANGLE or RUN_TARGET) or state of the speed integrator (all other modes) */
    count_t count_err_prev;        /**< Position error in the previous control iteration */
    ustime_t time_prev;            /**< Time at the previous control iteration */
    ustime_t time_started;         /**< Time that this maneuver/command/trajectory was started */
    ustime_t time_paused;          /**< The amount of time the speed integrator has spent paused */
    ustime_t time_stopped;         /**< Time at which the speed integrator last stopped */
    count_t integrator_ref_start;  /**< Integrated speed value prior to enabling integrator */
    count_t integrator_start;      /**< Integrated reference speed value prior to enabling integrator */
} pbio_motor_control_status_t;

// Initialize the current control status to being uninitialized
pbio_motor_control_status_t motor_control_status[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .windup_status = TIME_NOT_INITIALIZED
    }
};

// If the controller reach the maximum duty cycle value, this shortcut sets the stalled flag when the speed is below the stall limit.
void stall_set_flag_if_slow(stalled_status_t *stalled, rate_t rate_now, rate_t rate_limit, stalled_status_t flag){
    if (abs(rate_now) <= rate_limit) {
        // If the speed is less than the specified limit, set stalled flag.
        *stalled |= flag;
    }
    else {
        // Otherwise we are not yet stalled, so clear this flag.
        *stalled &= ~flag;
    }                 
}

// Clear the specified stall flag
void stall_clear_flag(stalled_status_t *stalled, stalled_status_t flag){
    *stalled &= ~flag;
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
pbio_error_t make_motor_trajectory(pbio_port_t port,
                                   pbio_motor_action_t action,
                                   int32_t speed_target,
                                   int32_t duration_or_target_position,
                                   pbio_motor_after_stop_t after_stop){

    // Read the current system state for this motor
    ustime_t time_start = pbdrv_time_get_usec();
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
    float_t _rate_target = speed_target * settings->counts_per_output_unit;
    float_t _accl_start = 0;
    float_t _accl_end = 0;

    // Limit reference rates
    _rate_start = limit(_rate_start, settings->max_rate);
    _rate_target = limit(_rate_target, settings->max_rate);

    // Set endpoint (time or angle), depending on selected action
    switch(action){
        case STOP:
            // For STOP, the end time is the current time plus the time needed for stopping
            _time_end = _time_start + abs(_rate_start)/((float_t) settings->abs_accl_end);
            break;
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
        // ... or the initial speed is zero when a stop is requested
        (action == STOP && rate_start == 0)
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
    // For a smooth stop, there is no acceleration and the constant speed phase is equal to the start rate
    else if (action == STOP) {
        _rate_target = _rate_start;
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
    if (action == STOP) {
        _time_in = _time_start;
        _count_in = _count_start;
    }
    else {
        _time_in = _time_start + (_rate_target-_rate_start)/_accl_start;
        _count_in = _count_start + ((_rate_target*_rate_target)-(_rate_start*_rate_start))/_accl_start/2;
    }

    // Compute intermediate time and angle values just before deceleration and end time or end angle, depending on which is already given
    if (action == RUN_TIME || action == STOP) {
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
    return PBIO_SUCCESS;
}

// Evaluate the reference speed and velocity at the (shifted) time
void get_reference(ustime_t time_ref, pbio_motor_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref){
    // For RUN and RUN_STALLED, the end time is infinite, meaning that the reference signals do not have a deceleration phase
    if (traject->action != STOP && time_ref - traject->time_in < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        *rate_ref = traject->rate_start   + timest(traject->accl_start, time_ref-traject->time_start);
        *count_ref = traject->count_start + timest(traject->rate_start, time_ref-traject->time_start) + timest2(traject->accl_start, time_ref-traject->time_start);
    }
    else if (traject->action != STOP && ((traject->action == RUN) || (traject->action == RUN_STALLED) || time_ref - traject->time_out <= 0)) {
        // If we are here, then we are in the constant speed phase
        *rate_ref = traject->rate_target;
        *count_ref = traject->count_in + timest(traject->rate_target, time_ref-traject->time_in);
    }
    else if ( time_ref - traject->time_end <= 0) {
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

void control_update(pbio_port_t port){
    // Trajectory and setting shortcuts for this motor
    pbio_motor_trajectory_t *traject = &trajectories[PORT_TO_IDX(port)];
    pbio_encmotor_settings_t *settings = &encmotor_settings[PORT_TO_IDX(port)];
    pbio_motor_control_status_t *status = &motor_control_status[PORT_TO_IDX(port)];
    duty_t max_duty = dcmotor_settings[PORT_TO_IDX(port)].max_stall_duty;

    // Return immediately if control is not active; then there is nothing we need to do
    if (motor_control_active[PORT_TO_IDX(port)] == PBIO_MOTOR_CONTROL_PASSIVE) {
        return;
    }

    // The very first time this is called, we initialize a fictitious previous maneuver
    if (status->windup_status == TIME_NOT_INITIALIZED) {
        status->time_started = traject->time_start - US_PER_SECOND;
    }

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_now, time_ref, time_loop;
    count_t count_now, count_ref, count_err;
    rate_t rate_now, rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Read current state of this motor: current time, speed, and position
    time_now = pbdrv_time_get_usec();
    pbio_encmotor_get_encoder_count(port, &count_now);
    pbio_encmotor_get_encoder_rate(port, &rate_now);

    // Check if the trajectory starting time equals the current maneuver start time
    if (traject->time_start != status->time_started) {
        // If not, then we are starting a new maneuver, and we update its starting time
        status->time_started = traject->time_start;

        // For this new maneuver, we reset PID variables and related persistent control settings
        status->err_integral = 0;
        status->time_paused = 0;
        status->time_stopped = 0;

        // Clear stalled status
        stall_clear_flag(&status->stalled, STALLED_PROPORTIONAL || STALLED_INTEGRAL);

        status->time_prev = status->time_started;
        status->count_err_prev = 0;
        // Reset control status flags
        if (traject->action == RUN_TARGET || traject->action == RUN_ANGLE){
            status->windup_status = TIME_RUNNING;
        }
        // else: RUN || RUN_TIME || RUN_STALLED || STOP
        else {
            status->windup_status = SPEED_INTEGRATOR_RUNNING;
            status->integrator_start = count_now;
            status->integrator_ref_start = traject->count_start;
        }
    }

    // Get the time at which we want to evaluate the reference position/velocities, for position based commands
    if (traject->action == RUN_TARGET || traject->action == RUN_ANGLE){
        // In nominal operation, take the current time, minus the amount of time we have spent stalled
        if (status->windup_status == TIME_RUNNING) {
            time_ref = time_now - status->time_paused;
        }
        else {
        // When the motor stalls, we keep the time constant. This way, the position reference does
        // not continue to grow unboundedly, thus preventing a form of wind-up
            time_ref = status->time_stopped - status->time_paused;
        }
    }
    // else: RUN || RUN_TIME || RUN_STALLED || STOP
    else {
        // For time based commands, we never pause the time; it is just the current time
        time_ref = time_now;
    }

    // Get reference signals
    get_reference(time_ref, traject, &count_ref, &rate_ref);

    // Position error
    if (traject->action == RUN_TARGET || traject->action == RUN_ANGLE){
        // For position based commands, this is just the reference minus the current value
        count_err = count_ref - count_now;
    }
    // else: RUN || RUN_TIME || RUN_STALLED || STOP
    else {
        // For time based commands, we do not aim to drive to a specific position, but we use the
        // "proportional position control" as an exact way to implement "integral speed control".
        // The speed integral is simply the position, but the speed reference should stop integrating
        // while stalled, to prevent windup.
        if (status->windup_status == SPEED_INTEGRATOR_RUNNING) {
            // If integrator is active, it is the previously accumulated sum, plus the integral since its last restart
            count_err = status->err_integral + count_ref - status->integrator_ref_start - count_now + status->integrator_start;
        }
        else {
            // Otherwise, it is just the previously accumulated sum and it doesn't integrate further
            count_err = status->err_integral;
        }
    }

    // For all commands, the speed error is simply the reference speed minus the current speed
    rate_err = rate_ref - rate_now;

    // Corresponding PD control signal
    duty_due_to_proportional = settings->pid_kp*count_err;
    duty_due_to_derivative = settings->pid_kd*rate_err;

    // Position anti-windup (RUN_TARGET || RUN_ANGLE)
    if (traject->action == RUN_TARGET || traject->action == RUN_ANGLE){
        if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)){
            // We are at the duty limit and we should prevent further position error "integration".
            // If we are additionally also running slower than the specified stall speed limit, set status to stalled
            stall_set_flag_if_slow(&status->stalled, rate_now, settings->stall_speed_limit, STALLED_PROPORTIONAL);
            // To prevent further integration, we should stop the timer if it is running
            if (status->windup_status == TIME_RUNNING) {
                // Then we must stop the time
                status->windup_status = TIME_PAUSED;
                // We save the time value reached now, to continue later
                status->time_stopped = time_now;
            }
        }
        else{
            // Otherwise, the timer should be running, and we are not stalled
            stall_clear_flag(&status->stalled, STALLED_PROPORTIONAL);
            // So we should restart the time if it isn't already running
            if (status->windup_status == TIME_PAUSED) {
                // Then we must restart the time
                status->windup_status = TIME_RUNNING;
                // We begin counting again from the stopped point
                status->time_paused += time_now - status->time_stopped;
            }
        }

        // Integrate position error
        if (status->windup_status == TIME_RUNNING) {
            time_loop = time_now - status->time_prev;
            status->err_integral += status->count_err_prev*time_loop;
        }
        status->count_err_prev = count_err;
        status->time_prev = time_now;
    }
    // Position anti-windup (RUN || RUN_TIME || RUN_STALLED || STOP)
    else {
        // Check if proportional control exceeds the duty limit
        if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)){
            // If we are additionally also running slower than the specified stall speed limit, set status to stalled
            stall_set_flag_if_slow(&status->stalled, rate_now, settings->stall_speed_limit, STALLED_PROPORTIONAL);
            // The integrator should NOT run.
            if (status->windup_status == SPEED_INTEGRATOR_RUNNING) {
                // If it is running, disable it
                status->windup_status = SPEED_INTEGRATOR_PAUSED;
                // Save the integrator state reached now, to continue when no longer stalled
                status->err_integral += count_ref - status->integrator_ref_start - count_now + status->integrator_start;
            }
        }
        else {
            stall_clear_flag(&status->stalled, STALLED_PROPORTIONAL);
            // The integrator SHOULD RUN.
            if (status->windup_status == SPEED_INTEGRATOR_PAUSED) {
                // If it isn't running, enable it
                status->windup_status = SPEED_INTEGRATOR_RUNNING;
                // Begin integrating again from the current point
                status->integrator_ref_start = count_ref;
                status->integrator_start = count_now;
            }
        }
    }

    // Duty cycle component due to integral position control (RUN_TARGET || RUN_ANGLE)
    if (traject->action == RUN_TARGET || traject->action == RUN_ANGLE){
        duty_due_to_integral = (settings->pid_ki*(status->err_integral/US_PER_MS))/MS_PER_SECOND;

        // Integrator anti windup (stalled in the sense of integrators)
        // Limit the duty due to the integral, as well as the integral itself
        if (duty_due_to_integral > max_duty) {
            // If we are additionally also running slower than the specified stall speed limit, set status to stalled
            stall_set_flag_if_slow(&status->stalled, rate_now, settings->stall_speed_limit, STALLED_INTEGRAL);            
            duty_due_to_integral = max_duty;
            status->err_integral = (US_PER_SECOND/settings->pid_ki)*max_duty;
        }
        else if (duty_due_to_integral < -max_duty) {
            stall_set_flag_if_slow(&status->stalled, rate_now, settings->stall_speed_limit, STALLED_INTEGRAL);
            duty_due_to_integral = -max_duty;
            status->err_integral = -(US_PER_SECOND/settings->pid_ki)*max_duty;
        }
        else {
            // Clear the integrator stall flag
            stall_clear_flag(&status->stalled, STALLED_INTEGRAL);
        }
    }
    else {
        // RUN || RUN_TIME || RUN_STALLED || STOP have no position integral control
        duty_due_to_integral = 0;
        stall_clear_flag(&status->stalled, STALLED_INTEGRAL);
    }

    // Calculate duty signal
    duty = duty_due_to_proportional + duty_due_to_integral + duty_due_to_derivative;

    // Check if we are at the target and standing still, with slightly different end conditions for each mode
    if (
        // Conditions for position based commands
        (
        (traject->action == RUN_TARGET || traject->action == RUN_ANGLE) &&
            (
                // Maneuver is complete, time wise
                time_ref >= traject->time_end &&
                // Position is within the lower tolerated bound ...
                count_ref - settings->tolerance <= count_now &&
                // ... and the upper tolerated bound
                count_now <= count_ref + settings->tolerance &&
                // And the motor stands still.
                abs(rate_now) < settings->min_rate
            )
        )
        ||
        // Conditions for RUN_TIME command
        (
        (traject->action == RUN_TIME) &&
            // We are past the total duration of the timed command
            (time_now >= traject->time_end)
        )
        ||
        // Conditions for time-based commands
        (
        (traject->action == STOP) &&
            // We are past the total duration of the timed command
            (time_now >= traject->time_end && abs(rate_now) < settings->min_rate)
        )
        ||
        // Conditions for run_stalled commands
        (
        (traject->action == RUN_STALLED) &&
            // Whether the motor is stalled in either proportional or integral sense
            (status->stalled != STALLED_NONE)
        )
    )
    {
    // If so, we have reached our goal. We can keep running this loop in order to hold this position.
    // But if brake or coast was specified as the afer_stop, we trigger that. Also clear the running flag to stop waiting for completion.
        if (traject->after_stop == PBIO_MOTOR_STOP_COAST){
            // Coast the motor
            pbio_dcmotor_coast(port);
        }
        else if (traject->after_stop == PBIO_MOTOR_STOP_BRAKE){
            // Brake the motor
            pbio_dcmotor_brake(port);
        }
        else if (traject->after_stop == PBIO_MOTOR_STOP_HOLD) {
            // Hold the motor
            if (traject->action == RUN_TARGET || traject->action == RUN_ANGLE){
                // In position based control, holding just means that we continue the position control loop without changes
                pbio_dcmotor_set_duty_cycle_int(port, duty);

                // Altough we keep holding, the maneuver is completed
                motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_HOLDING;
            }
            // RUN_TIME || RUN_STALLED || STOP
            else {
                // When ending a time based control maneuver with hold, we trigger a new position based maneuver with zero degrees
                pbio_dcmotor_set_duty_cycle_int(port, 0);
                make_motor_trajectory(port, RUN_TARGET, NONZERO, ((float_t) count_now)/settings->counts_per_output_unit, PBIO_MOTOR_STOP_HOLD);
            }
        }
    }
    // If we are not standing still at the target yet, actuate with the calculated signal
    else {
        pbio_dcmotor_set_duty_cycle_int(port, duty);
    }
}

#ifdef PBIO_CONFIG_ENABLE_MOTORS
// Service all the motors by calling this function at approximately constant intervals.
void _pbio_motorcontrol_poll(void){
    // Do the update for each motor
    for (pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++){
        control_update(port);
    }
}
#endif // PBIO_CONFIG_ENABLE_MOTORS

/* pbio user functions */

pbio_error_t pbio_encmotor_run(pbio_port_t port, float_t speed){
    pbio_error_t err = make_motor_trajectory(port, RUN, speed, NONE, NONE);
    if (err != PBIO_SUCCESS){
        return err;
    }
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_RUNNING;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_stop(pbio_port_t port, bool smooth, pbio_motor_after_stop_t after_stop){
    // Get current rate to see if we're standing still already
    float_t angle_now;
    rate_t rate_now;
    pbio_error_t err = pbio_encmotor_get_encoder_rate(port, &rate_now);
    if (err != PBIO_SUCCESS){
        return err;
    }
    if (smooth && rate_now != 0) {
        // Make a smooth controlled stop, equivalent to the last part of a RUN_TIME maneuver
        err = make_motor_trajectory(port, STOP, NONE, NONE, after_stop);
        if (err != PBIO_SUCCESS){
            return err;
        }
        motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_RUNNING;
        return PBIO_SUCCESS;
    }
    else {
        // Stop immediately, with one of the stop actions
        switch (after_stop) {
            case PBIO_MOTOR_STOP_COAST:
                // Stop by coasting
                err = pbio_dcmotor_coast(port);
                break;
            case PBIO_MOTOR_STOP_BRAKE:
                // Stop by braking
                err = pbio_dcmotor_brake(port);
                break;
            case PBIO_MOTOR_STOP_HOLD:
                // Force stop by holding the current position.
                // First, read where this position is
                err = pbio_encmotor_get_angle(port, &angle_now);
                if (err != PBIO_SUCCESS){
                    break;
                }
                // Holding is equivalent to driving to that position actively,
                // which automatically corrects the overshoot that is inevitable
                // when the user requests an immediate stop.
                make_motor_trajectory(port, RUN_TARGET, NONZERO, angle_now, after_stop);
                motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_RUNNING;
                break;
        }
        return err;
    }
}

pbio_error_t pbio_encmotor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop){
    pbio_error_t err = make_motor_trajectory(port, RUN_TIME, speed, duration, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_RUNNING;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_run_stalled(pbio_port_t port, float_t speed, pbio_motor_after_stop_t after_stop){
    pbio_error_t err = make_motor_trajectory(port, RUN_STALLED, speed, NONE, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_RUNNING;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop){
    pbio_error_t err = make_motor_trajectory(port, RUN_ANGLE, speed, angle, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_RUNNING;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop){
    pbio_error_t err = make_motor_trajectory(port, RUN_TARGET, speed, target, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_RUNNING;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_track_target(pbio_port_t port, float_t target){
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

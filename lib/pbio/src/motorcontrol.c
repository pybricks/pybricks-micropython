#include <pbio/motorcontrol.h>
#include <stdatomic.h>
#include <pbdrv/time.h>

// A "don't care" constant for readibility of the code, but which is never used after assignment
// Typically used for parameters that have no effect for the selected maneuvers.
#define NONE (0)

/**
 * Motor control actions
 */
typedef enum {
    IDLE,
    RUN,
    STOP,
    RUN_TIME,
    RUN_STALLED,
    RUN_TARGET,
    TRACK_TARGET,
} pbio_motor_action_t;

/**
 * Motor control command type
 */
typedef struct _pbio_motor_command_t {
    pbio_motor_action_t action;
    rate_t rate;
    int32_t duration_or_target_count;
    pbio_motor_after_stop_t after_stop;
    pbio_motor_wait_t wait;
} pbio_motor_command_t;

// Initialize new command to idle
pbio_motor_command_t command_new[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target_count = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

// Initialize current command to idle
pbio_motor_command_t command[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target_count = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

/**
 * Unsigned integer type with units of microseconds
 */
typedef uint32_t time_t;

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
 * Motor trajectory parameters for an ideal maneuver without disturbances
 */
typedef struct _pbio_motor_trajectory_t {
    bool forever;        // True if the maneuver has no defined endpoint
    time_t time_start;      // Time at start of maneuver
    time_t time_in;         // Time after the acceleration in-phase
    time_t time_out;        // Time at start of acceleration out-phase
    time_t time_end;        // Time at end of maneuver
    count_t count_start;    // Encoder count at start of maneuver
    count_t count_in;       // Encoder count after the acceleration in-phase
    count_t count_out;      // Encoder count at start of acceleration out-phase
    count_t count_end;      // Encoder count at end of maneuver
    rate_t rate_start;      // Encoder rate at start of maneuver
    rate_t rate_target;     // Encoder rate target when not accelerating
    accl_t accl_start;      // Encoder acceleration during in-phase
    accl_t accl_end;        // Encoder acceleration during out-phase
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

// Send a motor command to the task handler
pbio_error_t send_command(pbio_port_t port, pbio_motor_action_t action, rate_t speed, int32_t duration_or_target_count, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait);
// Store the motor command if it changed, and return true if it has
bool process_new_command(pbio_port_t port);

pbio_error_t pbio_encmotor_run(pbio_port_t port, float_t speed){
    float_t counts_per_output_unit = encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit;
    return send_command(port, RUN, (rate_t) (counts_per_output_unit * speed), NONE, NONE, NONE);
}

pbio_error_t pbio_encmotor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return PBIO_ERROR_FAILED; // Not yet implemented in theory
}

pbio_error_t pbio_encmotor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, RUN_TIME, (rate_t) (counts_per_output_unit * speed), (int32_t) (duration * US_PER_SECOND), after_stop, wait);
}

pbio_error_t pbio_encmotor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, RUN_STALLED, (rate_t) (counts_per_output_unit * speed), NONE, after_stop, wait);
    // Implement conditional waits... + conditional return
}

pbio_error_t pbio_encmotor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    count_t count;
    pbio_encmotor_get_encoder_count(port, &count);
    return send_command(port, RUN_TARGET, (rate_t) (counts_per_output_unit * speed), count + ((count_t) (counts_per_output_unit * angle)), after_stop, wait);
}

pbio_error_t pbio_encmotor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, RUN_TARGET, (rate_t) (counts_per_output_unit * speed), (count_t) (counts_per_output_unit * target), after_stop, wait);
}

pbio_error_t pbio_encmotor_track_target(pbio_port_t port, float_t target){
    return PBIO_ERROR_FAILED; // Not yet implemented in theory
}

void debug_command(pbio_port_t port){
    uint8_t idx = PORT_TO_IDX(port);
    printf("\nidx: %c\nact: %d\nspd: %d\nend: %d\nstp: %d\n", 
            idx + 'A',
            command[idx].action,
            command[idx].speed,
            (int) command[idx].duration_or_target_count,
            command[idx].after_stop
    );
}

// Return max(-limit, min(value, limit)): Limit the magnitude of value to be equal to or less than provided limit
int32_t limit(int32_t value, int32_t limit){
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

// Calculate the characteristic time values, encoder values, rate values and accelerations that uniquely define the rate and count trajectories
pbio_error_t get_trajectory_constants(pbio_motor_trajectory_t *traject, pbio_encmotor_settings_t *settings, pbio_motor_action_t action, time_t time_start, count_t count_start, rate_t rate_start, rate_t rate_target, int32_t duration_or_target_count){

    // Store characteristics that need no further computations
    traject->time_start = time_start;
    traject->count_start = count_start;

    // RUN and RUN_STALLED have no specific time or encoder based endpoint
    traject->forever = (action == RUN) || (action == RUN_STALLED);

    // RUN, STOP, RUN_TIME, RUN_STALLED are all specific cases of a generic timed control loop
    bool time_based = (action == RUN) || (action == RUN_TIME) || (action == RUN_STALLED);

    // RUN_ANGLE and RUN_TARGET are all specific cases of a generic position based control loop
    bool count_based = (action == RUN_TARGET);

    // Set the time endpoint for time based maneuvers if they are finite (corresponding count_end is computed from this)
    if (time_based) {
        // Do not allow negative time
        if (duration_or_target_count < 0) {
            return PBIO_ERROR_INVALID_ARG;
        }
        // For RUN_TIME, the end time is the current time plus the duration
        if (action == RUN_TIME) {
            traject->time_end = traject->time_start + ((time_t) duration_or_target_count);
        }
        // FOR RUN and RUN_STALLED, we specify no end time
        else {
            traject->time_end = NONE;
        }
    }

    // For position based maneuvers, we specify instead the end count value  (corresponding time_end is computed from this)
    if (count_based) {
        traject->count_end = (count_t) duration_or_target_count;
        // If the goal is to reach a position target, the speed cannot not be zero
        if (rate_target == 0) {
            return PBIO_ERROR_INVALID_ARG;
        }
    }

    // If the specified endpoint (angle or finite time) is equal to the corresponding starting value, return an empty maneuver.
    if ((count_based && traject->count_end == traject->count_start) || (action == RUN_TIME && traject->time_end == traject->time_start)) {
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
        return;
    }

    // Limit reference rates
    rate_start = limit(rate_start, settings->max_rate);
    rate_target = limit(rate_target, settings->max_rate);

    // Determine sign of reference rate in case of position target. The rate sign specified by the user is ignored
    if (count_based) {
        // If the target is ahead of us, go forward. Otherwise go backward.
        traject->rate_target = (traject->count_end > traject->count_start) ? abs(rate_target) : -abs(rate_target);
    }
    
    // To reduce complexity for now, we assume that the direction does not change during the acceleration phase.
    // If a reversal is requested, this therefore means an immediate reveral, and then a smooth acceleration to the
    // desired rate. This can be improved in future versions.
    if ((traject->rate_target < 0 && traject->rate_start > 0) || (traject->rate_target > 0 && traject->rate_start < 0)){
        traject->rate_start = 0;
    }

    // Accelerations with sign
    traject->accl_start = (traject->rate_target > traject->rate_start) ? settings->abs_accl_start : -settings->abs_accl_start;
    traject->accl_end = traject->rate_target > 0 ? -settings->abs_accl_end : settings->abs_accl_end;

    // Continue computations here...

    return PBIO_SUCCESS;
}


void motor_control_update(){

    uint32_t time_now;
    int32_t encoder_now;
    int32_t rate_now;

    // Do the update for each motor
    for (pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port < PBDRV_CONFIG_LAST_MOTOR_PORT; port++){
        // Port index
        uint8_t idx = PORT_TO_IDX(port);

        // Read the current time
        time_now = pbdrv_get_time_usec();

        // Read current position and speed
        pbio_encmotor_get_encoder_count(port, &encoder_now);
        pbio_encmotor_get_encoder_rate(port, &rate_now);

        if (process_new_command(port)){
            // Print out the newly set current command
            debug_command(port);

            int32_t endpoint = 0; // Todo

            // Generate reference trajectory parameters for new command
            get_trajectory_constants(&trajectories[idx], &encmotor_settings[idx], command[idx].action, time_now, encoder_now, rate_now, command[idx].speed, endpoint);
        }
        // Read current state of this motor: current time, speed, and position

        // Calculate control signal for current state and current command
        
        // Set the duty cycle
    }
}

/*

    Only the code below has to change once we use MICROPY_EVENT_POLL_HOOK to call the task handler

*/

// Atomic flag, one for each motor, that is set when the command_new variable is currently being read or being written. It is clear when it is free.
volatile atomic_flag busy[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

// Send a new command to the task handler
pbio_error_t send_command(pbio_port_t port, pbio_motor_action_t action, rate_t speed, int32_t duration_or_target_count, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    // Test if the motor is still available
    int32_t dummy;
    pbio_error_t error = pbio_encmotor_get_encoder_count(port, &dummy);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    uint8_t idx = PORT_TO_IDX(port);
    // Wait for the handler to free access to command variable, then claim it
    while(atomic_flag_test_and_set(&busy[idx]));
    // Set the new command
    command_new[idx].action = action;
    command_new[idx].speed = speed;
    command_new[idx].duration_or_target_count = duration_or_target_count;
    command_new[idx].after_stop = after_stop;
    // Release command variable
    atomic_flag_clear(&busy[idx]);
    return PBIO_SUCCESS;
}

bool process_new_command(pbio_port_t port){
    uint8_t idx = PORT_TO_IDX(port);
    bool haschanged = false;
    // Look for new command only if there is read access (otherwise, we'll check again next time)
    if (!atomic_flag_test_and_set(&busy[idx])) {
        // If we have read access, see if the command has changed since last time
        if (command[idx].action                   != command_new[idx].action                   ||
            command[idx].speed                    != command_new[idx].speed                    ||
            command[idx].duration_or_target_count != command_new[idx].duration_or_target_count ||
            command[idx].after_stop               != command_new[idx].after_stop)
        {
            // If the command changed, store that new command for non-atomic reading
            command[idx].action                   = command_new[idx].action;
            command[idx].speed                    = command_new[idx].speed;
            command[idx].duration_or_target_count = command_new[idx].duration_or_target_count;
            command[idx].after_stop               = command_new[idx].after_stop;
            haschanged = true;
        }
        atomic_flag_clear(&busy[idx]);
    }
    return haschanged;
}

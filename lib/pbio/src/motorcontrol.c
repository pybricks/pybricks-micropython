#include <pbio/motorcontrol.h>
#include <stdatomic.h>
#include <pbdrv/time.h>

#define NONE (0)
#define NaN (0)

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

/**
 * Motor control command type
 */
typedef struct _pbio_motor_command_t {
    pbio_motor_action_t action;
    int16_t speed;
    int32_t duration_or_target;
    pbio_motor_after_stop_t after_stop;
} pbio_motor_command_t;

// Initialize new command to idle
pbio_motor_command_t command_new[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

// Initialize current command to idle
pbio_motor_command_t command[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

typedef uint32_t time_t;
typedef uint32_t count_t;
typedef uint32_t rate_t;
typedef uint32_t accl_t;

/**
 * Motor trajectory parameters
 */
typedef struct _pbio_motor_trajectory_t {
    time_t time_start;
    time_t time_in;
    time_t time_out;
    time_t time_end;
    count_t count_start;
    count_t count_in;
    count_t count_out;
    count_t count_end;
    rate_t rate_start;
    rate_t rate_target;
    accl_t accl_in;
    accl_t accl_out;
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
        .accl_in = 0,
        .accl_out = 0
    }
};

// Atomic flag, one for each motor, that is set when the command_new variable is currently being read or being written. It is clear when it is free.
volatile atomic_flag busy[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

// Send a new command to the task handler
pbio_error_t send_command(uint8_t port, pbio_motor_action_t action, int16_t speed, int32_t duration_or_target, pbio_motor_after_stop_t after_stop){
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
    command_new[idx].duration_or_target = duration_or_target;
    command_new[idx].after_stop = after_stop;
    // Release command variable
    atomic_flag_clear(&busy[idx]);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_run(pbio_port_t port, float_t speed){
    float_t counts_per_output_unit = encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit;
    return send_command(port, RUN, (int16_t) (counts_per_output_unit * speed), NONE, NONE);
}

pbio_error_t pbio_encmotor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, STOP, NONE, NONE, NONE);
}

pbio_error_t pbio_encmotor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_encmotor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_encmotor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_encmotor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_encmotor_track_target(pbio_port_t port, float_t target){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

void debug_command(pbio_port_t port){
    uint8_t idx = PORT_TO_IDX(port);
    printf("\nidx: %c\nact: %d\nspd: %d\nend: %d\nstp: %d\n", 
            idx + 'A',
            command[idx].action,
            command[idx].speed,
            (int) command[idx].duration_or_target,
            command[idx].after_stop
    );
}

bool process_new_command(pbio_port_t port){
    uint8_t idx = PORT_TO_IDX(port);
    bool haschanged = false;
    // Look for new command only if there is read access (otherwise, we'll check again next time)
    if (!atomic_flag_test_and_set(&busy[idx])) {
        // If we have read access, see if the command has changed since last time
        if (command[idx].action             != command_new[idx].action             ||
            command[idx].speed              != command_new[idx].speed              ||
            command[idx].duration_or_target != command_new[idx].duration_or_target ||
            command[idx].after_stop         != command_new[idx].after_stop)
        {
            // If the command changed, store that new command for non-atomic reading
            command[idx].action             = command_new[idx].action;
            command[idx].speed              = command_new[idx].speed;
            command[idx].duration_or_target = command_new[idx].duration_or_target;
            command[idx].after_stop         = command_new[idx].after_stop;
            haschanged = true;
        }
        atomic_flag_clear(&busy[idx]);
    }
    return haschanged;
}


void compute_trajectory_constants(pbio_port_t port, pbio_motor_action_t action, time_t time_start, count_t count_start, rate_t rate_start, rate_t rate_target, uint32_t endpoint){
    
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
            compute_trajectory_constants(port, command[idx].action, time_now, encoder_now, rate_now, command[idx].speed, endpoint);
        }
        // Read current state of this motor: current time, speed, and position

        // Calculate control signal for current state and current command
        
        // Set the duty cycle
    }
}

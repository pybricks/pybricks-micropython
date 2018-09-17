#include <pbio/motorcontrol.h>
#include <stdatomic.h>
#include <pbdrv/time.h>

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
pbio_motor_command_t newcom[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

// Initialize current command to idle
pbio_motor_command_t curcom[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

// Atomic flag, one for each motor, that is set when the newcom variable is currently being read or being written. It is clear when it is free.
volatile atomic_flag busy[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

// Send a new command to the task handler
pbio_error_t send_command(uint8_t port, pbio_motor_action_t action, int16_t speed, int32_t duration_or_target, pbio_motor_after_stop_t after_stop){
    // Test if the motor is still available
    int32_t dummy;
    pbio_error_t error = pbio_motor_get_encoder_count(port, &dummy);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    uint8_t idx = PORT_TO_IDX(port);
    // Wait for the handler to free access to command variable, then claim it
    while(atomic_flag_test_and_set(&busy[idx]));
    // Set the new command
    newcom[idx].action = action;
    newcom[idx].speed = speed;
    newcom[idx].duration_or_target = duration_or_target;
    newcom[idx].after_stop = after_stop;
    // Release command variable
    atomic_flag_clear(&busy[idx]);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run(pbio_port_t port, float_t speed){
    float_t counts_per_output_unit = encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit;
    return send_command(port, RUN, (int16_t) (counts_per_output_unit * speed), NONE, NONE);
}

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, STOP, NONE, NONE, NONE);
}

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t target){
    return send_command(port, IDLE, NONE, NONE, NONE); // TODO
}

void debug_command(uint8_t idx){
    printf("\nidx: %c\nact: %d\nspd: %d\nend: %d\nstp: %d\n", 
            idx + 'A',
            curcom[idx].action,
            curcom[idx].speed,
            (int) curcom[idx].duration_or_target,
            curcom[idx].after_stop
    );
}

bool process_new_command(uint8_t idx){
    bool haschanged = false;
    // Look for new command only if there is read access (otherwise, we'll check again next time)
    if (!atomic_flag_test_and_set(&busy[idx])) {
        // If we have read access, see if the command has changed since last time
        if (curcom[idx].action             != newcom[idx].action             ||
            curcom[idx].speed              != newcom[idx].speed              ||
            curcom[idx].duration_or_target != newcom[idx].duration_or_target ||
            curcom[idx].after_stop         != newcom[idx].after_stop)
        {
            // If the command changed, store that new command for non-atomic reading
            curcom[idx].action             = newcom[idx].action;
            curcom[idx].speed              = newcom[idx].speed;
            curcom[idx].duration_or_target = newcom[idx].duration_or_target;
            curcom[idx].after_stop         = newcom[idx].after_stop;
            haschanged = true;
        }
        atomic_flag_clear(&busy[idx]);
    }
    return haschanged;
}

void motor_control_update(){

    uint32_t time_now;

    // Do the update for each motor
    for (uint8_t idx = 0; idx < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; idx++){

        // Read the current time
        pbdrv_get_time_usec(&time_now);

        if (process_new_command(idx)){
            // Print out the newly set current command
            debug_command(idx);

            // Generate reference trajectory parameters for new command
        }
        // Read current state of this motor: current time, speed, and position

        // Calculate control signal for current state and current command
        
        // Set the duty cycle
    }
}

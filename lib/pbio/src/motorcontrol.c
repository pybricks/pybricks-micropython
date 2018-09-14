#include <pbio/motorcontrol.h>

pbio_error_t pbio_motor_error(pbio_port_t port){
    int32_t dummy;
    return pbio_motor_get_encoder_count(port, &dummy);
}

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

typedef struct _pbio_motor_command_t {
    pbio_motor_action_t action;
    int16_t speed;
    int32_t duration_or_target;
    pbio_motor_after_stop_t after_stop;
} pbio_motor_command_t;

pbio_motor_command_t command[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

pbio_motor_command_t command_prev[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .speed = 0,
        .duration_or_target = 0,
        .after_stop = PBIO_MOTOR_STOP_COAST
    }
};

void debug_command(uint8_t idx){
    printf("\nidx: %d\nact: %d\nspd: %d\nend: %d\nstp: %d\n", 
            idx,
            command[idx].action,
            command[idx].speed,
            (int) command[idx].duration_or_target,
            command[idx].after_stop
    );
}

void remember_last_command(uint8_t idx){
    command_prev[idx].action = command[idx].action;
    command_prev[idx].speed = command[idx].speed;
    command_prev[idx].duration_or_target = command[idx].duration_or_target;
    command_prev[idx].after_stop = command[idx].after_stop;
}

bool command_changed(uint8_t idx){
    if (command_prev[idx].action != command[idx].action ||
        command_prev[idx].speed != command[idx].speed ||
        command_prev[idx].duration_or_target != command[idx].duration_or_target ||
        command_prev[idx].after_stop != command[idx].after_stop) {
        return true;
    }
    else{
        return false;
    }
}

pbio_error_t pbio_motor_run(pbio_port_t port, float_t speed){
    pbio_error_t error = pbio_motor_error(port);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    uint8_t idx = PORT_TO_IDX(port);
    command[idx].action = RUN;
    command[idx].speed = (int16_t) speed;
    command[idx].duration_or_target = 0;
    command[idx].after_stop = PBIO_MOTOR_STOP_COAST;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t error = pbio_motor_error(port);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    // uint8_t idx = PORT_TO_IDX(port);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t error = pbio_motor_error(port);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    // uint8_t idx = PORT_TO_IDX(port);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t error = pbio_motor_error(port);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    // uint8_t idx = PORT_TO_IDX(port);
    if (wait) {
        *stallpoint = 0.0;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t error = pbio_motor_error(port);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    // uint8_t idx = PORT_TO_IDX(port);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t error = pbio_motor_error(port);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    // uint8_t idx = PORT_TO_IDX(port);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t target){
    pbio_error_t error = pbio_motor_error(port);
    if (error != PBIO_SUCCESS) {
        return error;
    }
    // uint8_t idx = PORT_TO_IDX(port);
    return PBIO_SUCCESS;
}

void motor_control_update(){
    for (uint8_t idx = 0; idx < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; idx++){
        if (command_changed(idx)) {
            debug_command(idx);
            remember_last_command(idx);
            // Process new command

            // Generate reference trajectory parameters
        }
        // Calculate control signal
        
        // Set the duty cycle
    }
}

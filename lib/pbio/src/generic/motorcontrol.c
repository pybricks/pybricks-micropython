#include <pbio/motorcontrol.h>

pbio_error_t pbio_motor_run(pbio_port_t port, float_t gear_ratio, float_t speed){
    // TODO
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_stop(pbio_port_t port, float_t gear_ratio, pbio_motor_stop_t stop, pbio_motor_wait_t wait){
    // TODO
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t gear_ratio, float_t speed, float_t duration, pbio_motor_stop_t stop, pbio_motor_wait_t wait){
    // TODO
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t gear_ratio, float_t speed, float_t *stallpoint, pbio_motor_stop_t stop, pbio_motor_wait_t wait){
    // TODO
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t gear_ratio, float_t speed, float_t angle, pbio_motor_stop_t stop, pbio_motor_wait_t wait){
    // TODO
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t gear_ratio, float_t speed, float_t target, pbio_motor_stop_t stop, pbio_motor_wait_t wait){
    // TODO
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t gear_ratio, float_t target, pbio_motor_stop_t stop, pbio_motor_wait_t wait){
    // TODO
    return PBIO_SUCCESS;
}

void motorcontroller(){
    // TODO:
    // This will be the control task that is to be fired at approximately fixed intervals. 
    // This function does the hard work; the functions above just set/get values that
    // this control task uses to control the motors.
}
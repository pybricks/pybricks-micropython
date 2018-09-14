#include <pbio/motorcontrol.h>

pbio_error_t pbio_motor_run(pbio_port_t port, float_t speed){
    // TODO
    printf("run(port=%c, speed=%f)\n", port, speed);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    // TODO
    printf("stop(%c, after_stop=%d, wait=%d)\n", port, after_stop, wait);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    // TODO
    printf("run_time(port=%c, speed=%f, duration=%f, after_stop=%d, wait=%d)\n", port, speed, duration, after_stop, wait);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    // TODO
    printf("run_stalled(port=%c, speed=%f, after_stop=%d, wait=%d)\n", port, speed, after_stop, wait);
    if (wait) {
        *stallpoint = 0.0;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    // TODO
    printf("run_angle(port=%c, speed=%f, angle=%f, after_stop=%d, wait=%d)\n", port, speed, angle, after_stop, wait);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    // TODO
    printf("run_target(port=%c, speed=%f, target=%f, after_stop=%d, wait=%d)\n", port, speed, target, after_stop, wait);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t target){
    // TODO
    printf("track_target(port=%c, target=%f)\n", port, target);
    return PBIO_SUCCESS;
}



void motor_control_update(){
    // TODO:
    // This will be the control task that is to be fired at approximately fixed intervals. 
    // This function does the hard work; the functions above just set/get values that
    // this control task uses to control the motors.
}
#include <pbdrv/rawmotor.h>
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

/**
 * Control settings for an encoded motor
 */
typedef struct _pbio_encmotor_settings_t {
    int16_t counts_per_unit; /**< Encoder counts per output unit, including optional gear train (counts per degree for rotational motors, counts per cm for a linear motor) */
    float_t gear_ratio;             /**< Absolute slow down factor of an external gear train*/
    int16_t max_speed;              /**< Soft limit on the reference speed in all run commands */
    int16_t tolerance;              /**< Allowed deviation (deg) from target before motion is considered complete */
    int16_t acceleration_start;     /**< Acceleration when beginning to move. Positive value in degrees per second per second */
    int16_t acceleration_end;       /**< Deceleration when stopping. Positive value in degrees per second per second */
    int16_t tight_loop_time_ms;     /**< When a run function is called twice in this interval, assume that the user is doing their own speed control.  */
    int32_t offset;                 /**< Virtual zero point of the encoder */
    int16_t pid_kp;                 /**< Proportional position control constant (and integral speed control constant) */
    int16_t pid_ki;                 /**< Integral position control constant */
    int16_t pid_kd;                 /**< Derivative position control constant (and proportional speed control constant) */
} pbio_encmotor_settings_t;

pbio_encmotor_settings_t encmotor_settings[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_encmotor_setup(pbio_port_t port, pbio_id_t device_id, pbio_motor_dir_t direction, float_t gear_ratio){

    // Verify device ID and configure DC Motor
    pbio_error_t status = pbio_dcmotor_setup(port, device_id, direction);

    //
    // TODO: Verify that device_id is indeed an Encoded motor
    //

    //
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    //
    int16_t counts_per_unit = 1;
    pbio_encmotor_set_settings(port, 100.0, 1000, 1, 1000, 1000, 100, 5000, 5000, 50);

    // If all checks have passed, continue with setup of encoded motor
    if (status == PBIO_SUCCESS) {
        encmotor_settings[PORT_TO_IDX(port)].counts_per_unit = counts_per_unit;
        encmotor_settings[PORT_TO_IDX(port)].gear_ratio = gear_ratio;
        encmotor_settings[PORT_TO_IDX(port)].offset = 0;
        status = pbio_motor_reset_encoder_count(port, 0);
    }
    return status;
}

pbio_error_t pbio_encmotor_set_settings(
        pbio_port_t port,
        float_t stall_torque_limit,
        int16_t max_speed,
        int16_t tolerance,
        int16_t acceleration_start,
        int16_t acceleration_end,
        int16_t tight_loop_time_ms,
        int16_t pid_kp,
        int16_t pid_ki,
        int16_t pid_kd
    ){
    pbio_error_t status = pbio_dcmotor_set_settings(port, stall_torque_limit);
    if (status == PBIO_SUCCESS) {
        int8_t port_index = PORT_TO_IDX(port);
        encmotor_settings[port_index].max_speed = max_speed;
        encmotor_settings[port_index].tolerance = tolerance;
        encmotor_settings[port_index].acceleration_start = acceleration_start;
        encmotor_settings[port_index].acceleration_end = acceleration_end;
        encmotor_settings[port_index].tight_loop_time_ms = tight_loop_time_ms;
        encmotor_settings[port_index].pid_kp = pid_kp;
        encmotor_settings[port_index].pid_ki = pid_ki;
        encmotor_settings[port_index].pid_kd = pid_kd;       
    }
    return status;
};

void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string){
    int8_t port_index = PORT_TO_IDX(port);
    snprintf(settings_string, MAX_ENCMOTOR_SETTINGS_STR_LENGTH,
        "Counts per unit: %d\nGear ratio: %f\nMax speed: %d\nTolerance: %d\nAcceleration: %d\nDeceleration: %d\nTight Loop: %f\nkp: %f\nki: %f\nkd: %f",
        encmotor_settings[port_index].counts_per_unit,
        encmotor_settings[port_index].gear_ratio,            
        encmotor_settings[port_index].max_speed,
        encmotor_settings[port_index].tolerance,
        encmotor_settings[port_index].acceleration_start,
        encmotor_settings[port_index].acceleration_end,
        encmotor_settings[port_index].tight_loop_time_ms / MS_PER_SECOND,
        encmotor_settings[port_index].pid_kp / PID_PRESCALE,
        encmotor_settings[port_index].pid_ki / PID_PRESCALE,
        encmotor_settings[port_index].pid_kd / PID_PRESCALE     
    );
}

pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count) {
    pbio_error_t status = pbdrv_motor_get_encoder_count(port, count);
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *count = -*count;
    }    
    *count -= encmotor_settings[PORT_TO_IDX(port)].offset;
    return status;    
}

pbio_error_t pbio_motor_reset_encoder_count(pbio_port_t port, int32_t reset_count) {
    // First get the counter value without any offsets, but with the appropriate polarity/sign.
    int32_t count_no_offset;
    pbio_error_t status = pbio_motor_get_encoder_count(port, &count_no_offset);
    count_no_offset += encmotor_settings[PORT_TO_IDX(port)].offset;

    // Calculate the new offset
    encmotor_settings[PORT_TO_IDX(port)].offset = count_no_offset - reset_count;

    return status;
}

pbio_error_t pbio_motor_get_angle(pbio_port_t port, float_t *angle) {
    int32_t encoder_count;
    pbio_error_t status = pbio_motor_get_encoder_count(port, &encoder_count);
    *angle = encoder_count / (encmotor_settings[PORT_TO_IDX(port)].counts_per_unit * encmotor_settings[PORT_TO_IDX(port)].gear_ratio);
    return status;    
}

pbio_error_t pbio_motor_reset_angle(pbio_port_t port, float_t reset_angle) {
    // TODO: Abort any current maneuver
    return pbio_motor_reset_encoder_count(port, (int32_t) (reset_angle* encmotor_settings[PORT_TO_IDX(port)].counts_per_unit*encmotor_settings[PORT_TO_IDX(port)].gear_ratio));    
}

pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    pbio_error_t status = pbdrv_motor_get_encoder_rate(port, rate);
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *rate = -*rate;
    }    
    return status;    
}

pbio_error_t pbio_motor_get_angular_rate(pbio_port_t port, float_t *angular_rate) {
    int32_t encoder_rate;
    pbio_error_t status = pbio_motor_get_encoder_rate(port, &encoder_rate);
    *angular_rate = encoder_rate / (encmotor_settings[PORT_TO_IDX(port)].counts_per_unit * encmotor_settings[PORT_TO_IDX(port)].gear_ratio);
    return status;    
}

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

void motorcontroller(){
    // TODO:
    // This will be the control task that is to be fired at approximately fixed intervals. 
    // This function does the hard work; the functions above just set/get values that
    // this control task uses to control the motors.
}
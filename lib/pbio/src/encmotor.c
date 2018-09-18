#include <pbdrv/motor.h>
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

pbio_error_t pbio_encmotor_setup(pbio_port_t port, pbio_id_t device_id, pbio_motor_dir_t direction, float_t gear_ratio){

    // Verify device ID and configure DC Motor
    pbio_error_t status = pbio_dcmotor_setup(port, device_id, direction);

    //
    // TODO: Verify that device_id is indeed an Encoded motor
    //

    //
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    //
    float_t counts_per_unit = 1;

    // If all checks have passed, continue with setup of encoded motor
    if (status == PBIO_SUCCESS) {
        encmotor_settings[PORT_TO_IDX(port)].counts_per_unit = counts_per_unit;
        encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit = counts_per_unit * gear_ratio;
        encmotor_settings[PORT_TO_IDX(port)].offset = 0;
        status = pbio_encmotor_reset_encoder_count(port, 0);
    }
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    pbio_encmotor_set_settings(port, 100.0, 1000, 1, 1000, 1000, 100, 5, 5, 0.05);
    return status;
}

pbio_error_t pbio_encmotor_set_settings(
        pbio_port_t port,
        float_t stall_torque_limit,
        float_t max_speed,
        float_t tolerance,
        float_t acceleration_start,
        float_t acceleration_end,
        float_t tight_loop_time,
        float_t pid_kp,
        float_t pid_ki,
        float_t pid_kd
    ){
    pbio_error_t status = pbio_dcmotor_set_settings(port, stall_torque_limit);
    if (status == PBIO_SUCCESS) {
        int8_t port_index = PORT_TO_IDX(port);
        float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
        encmotor_settings[port_index].max_rate = (counts_per_output_unit * max_speed);
        encmotor_settings[port_index].tolerance = (counts_per_output_unit * tolerance);
        encmotor_settings[port_index].abs_accl_start = (counts_per_output_unit * acceleration_start);
        encmotor_settings[port_index].abs_accl_end = (counts_per_output_unit * acceleration_end);
        encmotor_settings[port_index].tight_loop_time_ms = (MS_PER_SECOND * tight_loop_time);
        encmotor_settings[port_index].pid_kp = (PID_PRESCALE * pid_kp);
        encmotor_settings[port_index].pid_ki = (PID_PRESCALE * pid_ki);
        encmotor_settings[port_index].pid_kd = (PID_PRESCALE * pid_kd);
    }
    return status;
};

void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string){
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    snprintf(settings_string, MAX_ENCMOTOR_SETTINGS_STR_LENGTH,
        "Counts per unit: %f\nGear ratio: %f\nMax speed: %f\nTolerance: %f\nAcceleration: %f\nDeceleration: %f\nTight Loop: %f\nkp: %f\nki: %f\nkd: %f",
        encmotor_settings[port_index].counts_per_unit,
        counts_per_output_unit / encmotor_settings[port_index].counts_per_output_unit,            
        encmotor_settings[port_index].max_rate / counts_per_output_unit,
        encmotor_settings[port_index].tolerance / counts_per_output_unit,
        encmotor_settings[port_index].abs_accl_start / counts_per_output_unit,
        encmotor_settings[port_index].abs_accl_end / counts_per_output_unit,
        ((float_t) encmotor_settings[port_index].tight_loop_time_ms) / MS_PER_SECOND,
        ((float_t) encmotor_settings[port_index].pid_kp) / PID_PRESCALE,
        ((float_t) encmotor_settings[port_index].pid_ki) / PID_PRESCALE,
        ((float_t) encmotor_settings[port_index].pid_kd) / PID_PRESCALE     
    );
}

pbio_error_t pbio_encmotor_get_encoder_count(pbio_port_t port, int32_t *count) {
    pbio_error_t status = pbdrv_motor_get_encoder_count(port, count);
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *count = -*count;
    }    
    *count -= encmotor_settings[PORT_TO_IDX(port)].offset;
    return status;    
}

pbio_error_t pbio_encmotor_reset_encoder_count(pbio_port_t port, int32_t reset_count) {
    // First get the counter value without any offsets, but with the appropriate polarity/sign.
    int32_t count_no_offset;
    pbio_error_t status = pbio_encmotor_get_encoder_count(port, &count_no_offset);
    count_no_offset += encmotor_settings[PORT_TO_IDX(port)].offset;

    // Calculate the new offset
    encmotor_settings[PORT_TO_IDX(port)].offset = count_no_offset - reset_count;

    return status;
}

pbio_error_t pbio_encmotor_get_angle(pbio_port_t port, float_t *angle) {
    int32_t encoder_count;
    pbio_error_t status = pbio_encmotor_get_encoder_count(port, &encoder_count);
    *angle = encoder_count / (encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit);
    return status;    
}

pbio_error_t pbio_encmotor_reset_angle(pbio_port_t port, float_t reset_angle) {
    // TODO: Abort any current maneuver
    return pbio_encmotor_reset_encoder_count(port, (int32_t) (reset_angle* encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit));    
}

pbio_error_t pbio_encmotor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    pbio_error_t status = pbdrv_motor_get_encoder_rate(port, rate);
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *rate = -*rate;
    }    
    return status;    
}

pbio_error_t pbio_encmotor_get_angular_rate(pbio_port_t port, float_t *angular_rate) {
    int32_t encoder_rate;
    pbio_error_t status = pbio_encmotor_get_encoder_rate(port, &encoder_rate);
    *angular_rate = encoder_rate / (encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit);
    return status;    
}

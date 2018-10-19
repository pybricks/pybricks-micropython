#include <pbdrv/motor.h>
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

pbio_error_t pbio_encmotor_setup(pbio_port_t port, pbio_id_t device_id, pbio_motor_dir_t direction, int16_t teeth_first, int16_t teeth_last){

    // Verify device ID and configure DC Motor
    pbio_error_t status = pbio_dcmotor_setup(port, device_id, direction);

    //
    // TODO: Verify that device_id is indeed an Encoded motor
    //

    //
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    //
    float_t counts_per_unit = 1.0;

    // If all checks have passed, continue with setup of encoded motor
    if (status == PBIO_SUCCESS) {
        encmotor_settings[PORT_TO_IDX(port)].counts_per_unit = counts_per_unit;
        encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit = (counts_per_unit * teeth_last)/teeth_first;
        encmotor_settings[PORT_TO_IDX(port)].offset = 0;
        status = pbio_encmotor_reset_encoder_count(port, 0);
    }
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    pbio_encmotor_set_settings(port, 100, 2, 5, 1000, 1, 1000, 1000, 0.1, 800, 800, 5);
    return status;
}

pbio_error_t pbio_encmotor_set_settings(
        pbio_port_t port,
        int16_t stall_torque_limit_pct,
        int32_t stall_speed_limit,
        int32_t min_speed,
        int32_t max_speed,
        int32_t tolerance,
        int32_t acceleration_start,
        int32_t acceleration_end,
        int16_t tight_loop_time,
        int16_t pid_kp,
        int16_t pid_ki,
        int16_t pid_kd
    ){
    pbio_error_t status = pbio_dcmotor_set_settings(port, stall_torque_limit_pct);
    if (status != PBIO_SUCCESS) {
        return status;
    }
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    encmotor_settings[port_index].stall_rate_limit = (counts_per_output_unit * stall_speed_limit);
    encmotor_settings[port_index].min_rate = (counts_per_output_unit * min_speed);
    encmotor_settings[port_index].max_rate = (counts_per_output_unit * max_speed);
    encmotor_settings[port_index].count_tolerance = (counts_per_output_unit * tolerance);
    encmotor_settings[port_index].abs_accl_start = (counts_per_output_unit * acceleration_start);
    encmotor_settings[port_index].abs_accl_end = (counts_per_output_unit * acceleration_end);
    encmotor_settings[port_index].tight_loop_time = tight_loop_time;
    encmotor_settings[port_index].pid_kp = pid_kp;
    encmotor_settings[port_index].pid_ki = pid_ki;
    encmotor_settings[port_index].pid_kd = pid_kd;
    return PBIO_SUCCESS;
};

void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string){
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    snprintf(settings_string, MAX_ENCMOTOR_SETTINGS_STR_LENGTH,
        "Counts per unit: %.2f\nGear ratio: %.2f\nStall speed: %d\nMin speed: %d\nMax speed: %d\nTolerance: %d\nAcceleration: %d\nDeceleration: %d\nTight Loop: %d\nkp: %d\nki: %d\nkd: %d",
        encmotor_settings[port_index].counts_per_unit,
        counts_per_output_unit / encmotor_settings[port_index].counts_per_unit,            
        (int) (encmotor_settings[port_index].stall_rate_limit / counts_per_output_unit),
        (int) (encmotor_settings[port_index].min_rate / counts_per_output_unit),
        (int) (encmotor_settings[port_index].max_rate / counts_per_output_unit),
        (int) (encmotor_settings[port_index].count_tolerance / counts_per_output_unit),
        (int) (encmotor_settings[port_index].abs_accl_start / counts_per_output_unit),
        (int) (encmotor_settings[port_index].abs_accl_end / counts_per_output_unit),
        encmotor_settings[port_index].tight_loop_time,
        encmotor_settings[port_index].pid_kp,
        encmotor_settings[port_index].pid_ki,
        encmotor_settings[port_index].pid_kd     
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
    // Set the motor to coast and stop any running maneuvers
    pbio_dcmotor_coast(port);

    // First get the counter value without any offsets, but with the appropriate polarity/sign.
    int32_t count_no_offset;
    pbio_error_t status = pbio_encmotor_get_encoder_count(port, &count_no_offset);
    count_no_offset += encmotor_settings[PORT_TO_IDX(port)].offset;

    // Calculate the new offset
    encmotor_settings[PORT_TO_IDX(port)].offset = count_no_offset - reset_count;

    return status;
}

pbio_error_t pbio_encmotor_get_angle(pbio_port_t port, int32_t *angle) {
    int32_t encoder_count;
    pbio_error_t status = pbio_encmotor_get_encoder_count(port, &encoder_count);
    *angle = encoder_count / (encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit);
    return status;    
}

pbio_error_t pbio_encmotor_reset_angle(pbio_port_t port, int32_t reset_angle) {
    return pbio_encmotor_reset_encoder_count(port, (int32_t) (reset_angle * encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit));    
}

pbio_error_t pbio_encmotor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    pbio_error_t status = pbdrv_motor_get_encoder_rate(port, rate);
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *rate = -*rate;
    }    
    return status;    
}

pbio_error_t pbio_encmotor_get_angular_rate(pbio_port_t port, int32_t *angular_rate) {
    int32_t encoder_rate;
    pbio_error_t status = pbio_encmotor_get_encoder_rate(port, &encoder_rate);
    *angular_rate = encoder_rate / (encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit);
    return status;    
}

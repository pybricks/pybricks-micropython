#include <pbio/dcmotor.h>

pbio_dcmotor_settings_t dcmotor_settings[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .direction = PBIO_MOTOR_DIR_NORMAL,
        .max_stall_duty = PBIO_MAX_DUTY
    }
};

pbio_error_t pbio_dcmotor_setup(pbio_port_t port, pbio_id_t device_id, pbio_motor_dir_t direction){   
    //
    // TODO: Verify that device_id matches device attached to port, else return appropriate erorr
    //
    printf("Class ID:%d\n", device_id);

    //
    // TODO: Verify that device_id is indeed a DC motor or inherited thereof (i.e. lpu_TrainMotor or ev3_LargeMotor etc), else return appropriate erorr
    //

    pbio_error_t status = pbdrv_motor_coast(port);
    if (status == PBIO_SUCCESS) {
        dcmotor_settings[PORT_TO_IDX(port)].direction = direction;
    }

    //
    // TODO: Use the device_id to set the default settings defined in our lib. For now just hardcode something below.
    //
    pbio_dcmotor_set_settings(port, 100.0);

    return status;
}

pbio_error_t pbio_dcmotor_set_settings(pbio_port_t port, float_t stall_torque_limit){
    pbio_error_t status = pbdrv_motor_coast(port);
    int16_t max_stall_duty = (int16_t) (PBIO_DUTY_PCT_TO_ABS * stall_torque_limit);
    if (max_stall_duty < 0 || max_stall_duty > PBIO_MAX_DUTY) {
        status = PBIO_ERROR_INVALID_ARG;
    }
    if (status == PBIO_SUCCESS) { 
        dcmotor_settings[PORT_TO_IDX(port)].max_stall_duty = max_stall_duty;
    }
    return status;
}

void pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string){
    int8_t port_index = PORT_TO_IDX(port);
    char format_string [] = "Port: %c\nDirection: %s\nTorque limit: %f";
    if (dcmotor_settings[port_index].direction == PBIO_MOTOR_DIR_NORMAL) {
        snprintf(settings_string, MAX_DCMOTOR_SETTINGS_STR_LENGTH, format_string, port, "normal", dcmotor_settings[port_index].max_stall_duty / PBIO_DUTY_PCT_TO_ABS);
    }
    else{
        snprintf(settings_string, MAX_DCMOTOR_SETTINGS_STR_LENGTH, format_string, port, "inverted", dcmotor_settings[port_index].max_stall_duty / PBIO_DUTY_PCT_TO_ABS);
    }        
}

pbio_error_t pbio_dcmotor_coast(pbio_port_t port){
    return pbdrv_motor_coast(port);
}

pbio_error_t pbio_dcmotor_brake(pbio_port_t port){
    return pbdrv_motor_set_duty_cycle(port, 0);
}

pbio_error_t pbio_dcmotor_set_duty_cycle_int(pbio_port_t port, int32_t duty_cycle_int) {
    // Limit the duty cycle value
    int16_t limit = dcmotor_settings[PORT_TO_IDX(port)].max_stall_duty;
    if (duty_cycle_int > limit) {
        duty_cycle_int = limit;
    }
    if (duty_cycle_int < -limit) {
        duty_cycle_int = -limit;
    }
    // Flip sign if motor is inverted
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED){
        duty_cycle_int = -duty_cycle_int;
    }
    return pbdrv_motor_set_duty_cycle(port, duty_cycle_int);
}

pbio_error_t pbio_dcmotor_set_duty_cycle(pbio_port_t port, float_t duty_cycle) {
    return pbio_dcmotor_set_duty_cycle_int(port, PBIO_DUTY_PCT_TO_ABS * duty_cycle);
}

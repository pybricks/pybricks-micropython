// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <inttypes.h>

#include <pbdrv/motor.h>
#include <pbio/motor.h>

// Initialize motor control state as inactive
pbio_motor_control_active_t motor_control_active[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)] PBIO_MOTOR_CONTROL_PASSIVE
};

pbio_error_t pbio_dcmotor_setup(pbio_port_t port, pbio_motor_dir_t direction){
    // Get the ID according to IODEV
    // pbio_iodev_type_id_t auto_id =

    pbio_error_t status = pbio_dcmotor_coast(port);
    if (status != PBIO_SUCCESS && status != PBIO_ERROR_IO) {
        return status;
    }
    motor_directions[PORT_TO_IDX(port)] = direction;
    motor_has_encoders[PORT_TO_IDX(port)] = false;

    return PBIO_SUCCESS;
}

void pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string){
    char *direction = motor_directions[PORT_TO_IDX(port)] == PBIO_MOTOR_DIR_CLOCKWISE ? "clockwise" : "counterclockwise";
    snprintf(settings_string, MAX_DCMOTOR_SETTINGS_STR_LENGTH,
        "Motor properties:\n"
        "------------------------\n"
        "Port\t\t %c\n"
        "Direction\t %s",
        port,
        direction
    );
}

pbio_error_t pbio_dcmotor_coast(pbio_port_t port){
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_PASSIVE;
    return pbdrv_motor_coast(port);
}

pbio_error_t pbio_dcmotor_brake(pbio_port_t port){
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_PASSIVE;
    return pbdrv_motor_set_duty_cycle(port, 0);
}

pbio_error_t pbio_dcmotor_set_duty_cycle_sys(pbio_port_t port, int32_t duty_steps) {
    // Limit the duty cycle value
    int32_t limit = encmotor_settings[PORT_TO_IDX(port)].max_duty_steps;
    if (duty_steps > limit) {
        duty_steps = limit;
    }
    if (duty_steps < -limit) {
        duty_steps = -limit;
    }
    int32_t duty_cycle;
    // Add the configured offset and scale remaining duty
    if (duty_steps == 0) {
        duty_cycle = 0;
    }
    else {
        int32_t offset = encmotor_settings[PORT_TO_IDX(port)].duty_offset;
        int32_t offset_signed = duty_steps > 0 ? offset : -offset;
        duty_cycle = offset_signed + ((PBIO_DUTY_STEPS-offset)*duty_steps)/PBIO_DUTY_STEPS;
    }
    // Flip sign if motor is inverted
    if (motor_directions[PORT_TO_IDX(port)] == PBIO_MOTOR_DIR_COUNTERCLOCKWISE){
        duty_cycle = -duty_cycle;
    }
    return pbdrv_motor_set_duty_cycle(port, duty_cycle);
}

pbio_error_t pbio_dcmotor_set_duty_cycle_usr(pbio_port_t port, float_t duty_steps) {
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_PASSIVE;
    return pbio_dcmotor_set_duty_cycle_sys(port, PBIO_DUTY_STEPS_PER_USER_STEP * duty_steps);
}

pbio_error_t pbio_motor_setup(pbio_port_t port, pbio_motor_dir_t direction, float_t gear_ratio) {

    // Configure DC Motor
    pbio_error_t status = pbio_dcmotor_setup(port, direction);
    if (status != PBIO_SUCCESS) {
        return status;
    }
    // Reset encoder, and in the process check that it is an encoded motor
    motor_has_encoders[PORT_TO_IDX(port)] = pbio_encmotor_reset_encoder_count(port, 0) == PBIO_SUCCESS;

    // Return if there are no encoders, because then we are done
    if (!motor_has_encoders[PORT_TO_IDX(port)]) {
        return PBIO_SUCCESS;
    }

    // Return on invalid gear ratio
    if (gear_ratio <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    pbio_iodev_type_id_t id;
    status = pbdrv_motor_get_id(port, &id);
    if (status != PBIO_SUCCESS) {
        return status;
    }

    //
    // TODO: Use the device_id to retrieve this number. It is 1.0 for all of the supported motors so far.
    // It is 2.0 for motors with double resolution, and it is counts/mm for linear actuators.
    //
    float_t counts_per_unit = 1.0;

    // Overal ratio between encoder counts and output
    float_t ratio = counts_per_unit * gear_ratio;

    encmotor_settings[PORT_TO_IDX(port)].counts_per_unit = counts_per_unit;
    encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit = ratio;

    // TODO: Load data by ID rather than hardcoding here, and define shared defaults to reduce size
    if (id == PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR) {
        pbio_encmotor_set_dc_settings(port, 100, 0);
        pbio_encmotor_set_run_settings(port, 1200/ratio, 2400/ratio);
        pbio_encmotor_set_pid_settings(port, 400, 600, 5, 100, 3, 5, 2, 200);
    }
    else if (id == PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR) {
        pbio_encmotor_set_dc_settings(port, 100, 0);
        pbio_encmotor_set_run_settings(port, 800/ratio, 1600/ratio);
        pbio_encmotor_set_pid_settings(port, 500, 800, 5, 100, 3, 5, 2, 200);
    }
    else if (id == PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR) {
        pbio_encmotor_set_dc_settings(port, 100, 0);
        pbio_encmotor_set_run_settings(port, 1500/ratio, 3000/ratio);
        pbio_encmotor_set_pid_settings(port, 400, 600, 5, 100, 3, 5, 2, 200);
    }
    else {
        // Defaults
        pbio_encmotor_set_dc_settings(port, 100, 0);
        pbio_encmotor_set_run_settings(port, 1000/ratio, 1000/ratio);
        pbio_encmotor_set_pid_settings(port, 500, 800, 5, 100, 3, 5, 2, 500);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_set_dc_settings(pbio_port_t port, int32_t stall_torque_limit_pct, int32_t duty_offset_pct) {

    if (stall_torque_limit_pct < 0 || duty_offset_pct < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    int8_t port_index = PORT_TO_IDX(port);
    encmotor_settings[port_index].max_duty_steps = PBIO_DUTY_STEPS_PER_USER_STEP * stall_torque_limit_pct;
    encmotor_settings[port_index].duty_offset = PBIO_DUTY_STEPS_PER_USER_STEP * duty_offset_pct;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_get_dc_settings(pbio_port_t port, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct) {
    int8_t port_index = PORT_TO_IDX(port);
    *stall_torque_limit_pct = encmotor_settings[port_index].max_duty_steps/PBIO_DUTY_STEPS_PER_USER_STEP;
    *duty_offset_pct = encmotor_settings[port_index].duty_offset/PBIO_DUTY_STEPS_PER_USER_STEP;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_set_run_settings(pbio_port_t port, int32_t max_speed, int32_t acceleration) {
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    encmotor_settings[port_index].max_rate = (counts_per_output_unit * max_speed);
    encmotor_settings[port_index].abs_acceleration = (counts_per_output_unit * acceleration);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_set_pid_settings(
        pbio_port_t port,
        int16_t pid_kp,
        int16_t pid_ki,
        int16_t pid_kd,
        int32_t tight_loop_time,
        int32_t position_tolerance,
        int32_t speed_tolerance,
        int32_t stall_speed_limit,
        int32_t stall_time) {
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;

    if (pid_kp < 0 || pid_ki < 0 || pid_kd < 0 || tight_loop_time < 0 ||
        position_tolerance < 0 || speed_tolerance < 0 || stall_speed_limit < 0 || stall_time < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    encmotor_settings[port_index].pid_kp = pid_kp;
    encmotor_settings[port_index].pid_ki = pid_ki;
    encmotor_settings[port_index].pid_kd = pid_kd;
    encmotor_settings[port_index].tight_loop_time = tight_loop_time * US_PER_MS;
    encmotor_settings[port_index].count_tolerance = (counts_per_output_unit * position_tolerance);
    encmotor_settings[port_index].rate_tolerance = (counts_per_output_unit * speed_tolerance);
    encmotor_settings[port_index].stall_rate_limit = (counts_per_output_unit * stall_speed_limit);
    encmotor_settings[port_index].stall_time = stall_time * US_PER_MS;
    return PBIO_SUCCESS;
}

void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string){
    // Preload several settings for easier printing
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    float_t counts_per_unit = encmotor_settings[port_index].counts_per_unit;
    float_t gear_ratio = counts_per_output_unit / encmotor_settings[port_index].counts_per_unit;
    // Print settings to settings_string
    snprintf(settings_string, MAX_ENCMOTOR_SETTINGS_STR_LENGTH,
        "Counts per unit\t %" PRId32 ".%" PRId32 "\n"
        "Gear ratio\t %" PRId32 ".%" PRId32 "\n"
        "\nRun settings:\n"
        "------------------------\n"
        "Max speed\t %" PRId32 "\n"
        "Acceleration\t %" PRId32 "\n"
        "\nDC settings:\n"
        "------------------------\n"
        "Duty limit\t %" PRId32 "\n"
        "Duty offset\t %" PRId32 "\n"
        "\nPID settings:\n"
        "------------------------\n"
        "kp\t\t %" PRId32 "\n"
        "ki\t\t %" PRId32 "\n"
        "kd\t\t %" PRId32 "\n"
        "Tight Loop\t %" PRId32 "\n"
        "Angle tolerance\t %" PRId32 "\n"
        "Speed tolerance\t %" PRId32 "\n"
        "Stall speed\t %" PRId32 "\n"
        "Stall time\t %" PRId32,
        // Print counts_per_unit as floating point with 3 decimals
        (int32_t) (counts_per_unit),
        (int32_t) (counts_per_unit*1000 - ((int32_t) counts_per_unit)*1000),
        // Print counts_per_unit as floating point with 3 decimals
        (int32_t) (gear_ratio),
        (int32_t) (gear_ratio*1000 - ((int32_t) gear_ratio)*1000),
        // Print run settings
        (int32_t) (encmotor_settings[port_index].max_rate / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].abs_acceleration / counts_per_output_unit),
        // Print DC settings
        (int32_t) (encmotor_settings[port_index].max_duty_steps / PBIO_DUTY_STEPS_PER_USER_STEP),
        (int32_t) (encmotor_settings[port_index].duty_offset / PBIO_DUTY_STEPS_PER_USER_STEP),
        // Print PID settings
        (int32_t) encmotor_settings[port_index].pid_kp,
        (int32_t) encmotor_settings[port_index].pid_ki,
        (int32_t) encmotor_settings[port_index].pid_kd,
        (int32_t) (encmotor_settings[port_index].tight_loop_time / US_PER_MS),
        (int32_t) (encmotor_settings[port_index].count_tolerance / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].rate_tolerance / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].stall_rate_limit / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].stall_time  / US_PER_MS)
    );
}

bool pbio_encmotor_has_encoder(pbio_port_t port){
    return motor_has_encoders[PORT_TO_IDX(port)];
}

pbio_error_t pbio_encmotor_get_encoder_count(pbio_port_t port, int32_t *count) {
    pbio_error_t status = pbdrv_motor_get_encoder_count(port, count);
    if (motor_directions[PORT_TO_IDX(port)] == PBIO_MOTOR_DIR_COUNTERCLOCKWISE) {
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
    if (motor_directions[PORT_TO_IDX(port)] == PBIO_MOTOR_DIR_COUNTERCLOCKWISE) {
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

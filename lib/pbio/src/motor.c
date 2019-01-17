/*
 * Copyright (c) 2018 Laurens Valk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

pbio_error_t pbio_dcmotor_set_duty_cycle_int(pbio_port_t port, int32_t duty_cycle_int) {
    // Limit the duty cycle value
    int32_t limit = encmotor_settings[PORT_TO_IDX(port)].max_stall_duty;
    if (duty_cycle_int > limit) {
        duty_cycle_int = limit;
    }
    if (duty_cycle_int < -limit) {
        duty_cycle_int = -limit;
    }
    // Flip sign if motor is inverted
    if (motor_directions[PORT_TO_IDX(port)] == PBIO_MOTOR_DIR_COUNTERCLOCKWISE){
        duty_cycle_int = -duty_cycle_int;
    }
    return pbdrv_motor_set_duty_cycle(port, duty_cycle_int);
}

pbio_error_t pbio_dcmotor_set_duty_cycle(pbio_port_t port, float_t duty_cycle, float_t duty_limit) {
    duty_limit = duty_limit >= 0 ? duty_limit : -duty_limit;
    if (duty_cycle > duty_limit) {
        duty_cycle = duty_limit;
    }
    else if (duty_cycle < -duty_limit) {
        duty_cycle = -duty_limit;
    }
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_PASSIVE;
    return pbio_dcmotor_set_duty_cycle_int(port, PBIO_DUTY_PCT_TO_ABS * duty_cycle);
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

    //
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    //
    float_t counts_per_unit = 1.0;

    encmotor_settings[PORT_TO_IDX(port)].counts_per_unit = counts_per_unit;
    encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit = counts_per_unit * gear_ratio;

    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    pbio_encmotor_set_run_settings(port, 1000, 1000);

    // TODO: Add quick hack to distinguish between ev3 large/small
    pbio_encmotor_set_pid_settings(port, 500, 800, 5, 100, 3, 5);

    pbio_encmotor_set_stall_settings(port, 100, 2, 500);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_set_run_settings(pbio_port_t port, int32_t max_speed, int32_t acceleration) {
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    encmotor_settings[port_index].max_rate = (counts_per_output_unit * max_speed);
    encmotor_settings[port_index].abs_acceleration = (counts_per_output_unit * acceleration);
    return PBIO_SUCCESS;
};

pbio_error_t pbio_encmotor_set_pid_settings(pbio_port_t port, int16_t pid_kp, int16_t pid_ki, int16_t pid_kd, int32_t tight_loop_time, int32_t position_tolerance, int32_t speed_tolerance) {
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    encmotor_settings[port_index].pid_kp = pid_kp;
    encmotor_settings[port_index].pid_ki = pid_ki;
    encmotor_settings[port_index].pid_kd = pid_kd;
    encmotor_settings[port_index].tight_loop_time = tight_loop_time * US_PER_MS;
    encmotor_settings[port_index].count_tolerance = (counts_per_output_unit * position_tolerance);
    encmotor_settings[port_index].rate_tolerance = (counts_per_output_unit * speed_tolerance);
    return PBIO_SUCCESS;
};

pbio_error_t pbio_encmotor_set_stall_settings(
        pbio_port_t port,
        int32_t stall_torque_limit_pct,
        int32_t stall_speed_limit,
        int32_t stall_time
    ){
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    encmotor_settings[port_index].max_stall_duty = PBIO_DUTY_PCT_TO_ABS * stall_torque_limit_pct;
    encmotor_settings[port_index].stall_rate_limit = (counts_per_output_unit * stall_speed_limit);
    encmotor_settings[port_index].stall_time = stall_time * US_PER_MS;
    return PBIO_SUCCESS;
};

pbio_error_t pbio_encmotor_get_stall_settings(
        pbio_port_t port,
        int32_t *stall_torque_limit_pct,
        int32_t *stall_speed_limit,
        int32_t *stall_time
    ){
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    *stall_torque_limit_pct = encmotor_settings[port_index].max_stall_duty/PBIO_DUTY_PCT_TO_ABS;
    *stall_speed_limit = encmotor_settings[port_index].stall_rate_limit/counts_per_output_unit;
    *stall_time = encmotor_settings[port_index].stall_time/US_PER_MS;
    return PBIO_SUCCESS;
};

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
        "Stall speed\t %" PRId32 "\n"
        "Stall time\t %" PRId32 "\n"
        "Speed tolerance\t %" PRId32 "\n"
        "Max speed\t %" PRId32 "\n"
        "Angle tolerance\t %" PRId32 "\n"
        "Acceleration\t %" PRId32 "\n"
        "Tight Loop\t %" PRId32 "\n"
        "kp\t\t %" PRId32 "\n"
        "ki\t\t %" PRId32 "\n"
        "kd\t\t %" PRId32 "",
        // Print counts_per_unit as floating point with 3 decimals
        (int32_t) (counts_per_unit),
        (int32_t) (counts_per_unit*1000 - ((int32_t) counts_per_unit)*1000),
        // Print counts_per_unit as floating point with 3 decimals
        (int32_t) (gear_ratio),
        (int32_t) (gear_ratio*1000 - ((int32_t) gear_ratio)*1000),
        // Print remaining settings as integers
        (int32_t) (encmotor_settings[port_index].stall_rate_limit / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].stall_time  / US_PER_MS),
        (int32_t) (encmotor_settings[port_index].rate_tolerance / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].max_rate / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].count_tolerance / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].abs_acceleration / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].tight_loop_time / US_PER_MS),
        (int32_t) encmotor_settings[port_index].pid_kp,
        (int32_t) encmotor_settings[port_index].pid_ki,
        (int32_t) encmotor_settings[port_index].pid_kd
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

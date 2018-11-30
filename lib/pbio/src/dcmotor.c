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

#include <pbio/dcmotor.h>
#include <inttypes.h>

// Initialize DC Motor settings
pbio_dcmotor_settings_t dcmotor_settings[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .direction = PBIO_MOTOR_DIR_NORMAL,
        .max_stall_duty = PBIO_MAX_DUTY
    }
};

// Initialize motor control state as inactive
pbio_motor_control_active_t motor_control_active[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)] PBIO_MOTOR_CONTROL_PASSIVE
};

pbio_error_t pbio_dcmotor_setup(pbio_port_t port, pbio_motor_dir_t direction){
    // Get the ID according to IODEV
    // pbio_iodev_type_id_t auto_id =

    pbio_error_t status = pbio_dcmotor_coast(port);
    if (status != PBIO_SUCCESS) {
        return status;
    }
    dcmotor_settings[PORT_TO_IDX(port)].direction = direction;

    //
    // TODO: Use the auto_id to set the default settings defined in our lib. For now just hardcode something below.
    //
    pbio_dcmotor_set_settings(port, 100);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_set_settings(pbio_port_t port, int16_t stall_torque_limit_pct){
    pbio_error_t status = pbio_dcmotor_coast(port);
    if (stall_torque_limit_pct < 0 || stall_torque_limit_pct > PBIO_MAX_DUTY_PCT) {
        stall_torque_limit_pct = PBIO_MAX_DUTY_PCT;
    }
    dcmotor_settings[PORT_TO_IDX(port)].max_stall_duty = PBIO_DUTY_PCT_TO_ABS * stall_torque_limit_pct;
    return status;
}

void pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string){
    int8_t port_index = PORT_TO_IDX(port);
    char *direction = dcmotor_settings[port_index].direction == PBIO_MOTOR_DIR_NORMAL ? "normal" : "inverted";
    snprintf(settings_string, MAX_DCMOTOR_SETTINGS_STR_LENGTH,
        "Port\t\t %c\n"
        "Direction\t %s\n"
        "Torque limit\t %" PRId32 "",
        port,
        direction,
        (int32_t) (dcmotor_settings[port_index].max_stall_duty / PBIO_DUTY_PCT_TO_ABS)
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
    int32_t limit = dcmotor_settings[PORT_TO_IDX(port)].max_stall_duty;
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
    motor_control_active[PORT_TO_IDX(port)] = PBIO_MOTOR_CONTROL_PASSIVE;
    return pbio_dcmotor_set_duty_cycle_int(port, PBIO_DUTY_PCT_TO_ABS * duty_cycle);
}

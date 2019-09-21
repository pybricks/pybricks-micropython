// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_SERVO_H_
#define _PBIO_SERVO_H_

#include <stdint.h>
#include <stdio.h>

#include <fixmath.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>
#include <pbdrv/counter.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/dc.h>
#include <pbio/tacho.h>
#include <pbio/trajectory.h>
#include <pbio/control.h>

#include <pbio/iodev.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (128)

#define MAX_ENCMOTOR_SETTINGS_STR_LENGTH (400)

#define PBIO_DUTY_STEPS (PBDRV_MAX_DUTY)
#define PBIO_DUTY_USER_STEPS (100)
#define PBIO_DUTY_STEPS_PER_USER_STEP (PBIO_DUTY_STEPS/PBIO_DUTY_USER_STEPS)


typedef struct _pbio_servo_t {
    pbio_dc_t dc;
    pbio_tacho_t *tacho;
    pbio_motor_state_t state;
    pbio_control_t control;
    pbio_port_t port;
} pbio_servo_t;

pbio_error_t pbio_dc_coast(pbio_servo_t *mtr);
pbio_error_t pbio_dc_brake(pbio_servo_t *mtr);
pbio_error_t pbio_dc_set_duty_cycle_sys(pbio_servo_t *mtr, int32_t duty_steps);
pbio_error_t pbio_dc_set_duty_cycle_usr(pbio_servo_t *mtr, int32_t duty_steps);

void _pbio_servo_init(void);
void _pbio_servo_poll(void);
pbio_error_t pbio_dc_set_settings(pbio_servo_t *mtr, int32_t stall_torque_limit_pct, int32_t duty_offset_pct);
pbio_error_t pbio_dc_get_settings(pbio_servo_t *mtr, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct);

pbio_error_t pbio_tacho_get_count(pbio_tacho_t *tacho, int32_t *count);
pbio_error_t pbio_tacho_reset_count(pbio_tacho_t *tacho, int32_t reset_count);
pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, int32_t *angle);
pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, int32_t reset_angle);
pbio_error_t pbio_tacho_get_rate(pbio_tacho_t *tacho, int32_t *encoder_rate);
pbio_error_t pbio_tacho_get_angular_rate(pbio_tacho_t *tacho, int32_t *angular_rate);

pbio_error_t pbio_servo_get(pbio_port_t port, pbio_servo_t **mtr);  // TODO: Make dc and servo version
pbio_error_t pbio_servo_setup(pbio_servo_t *mtr, pbio_direction_t direction, fix16_t gear_ratio);
void pbio_servo_print_settings(pbio_servo_t *mtr, char *dc_settings_string, char *enc_settings_string);
pbio_error_t pbio_servo_reset_angle(pbio_servo_t *mtr, int32_t reset_angle);
pbio_error_t pbio_servo_is_stalled(pbio_servo_t *mtr, bool *stalled);
pbio_error_t pbio_servo_run(pbio_servo_t *mtr, int32_t speed);
pbio_error_t pbio_servo_stop(pbio_servo_t *mtr, pbio_control_after_stop_t after_stop); // TODO: Make dc and servo version
pbio_error_t pbio_servo_run_time(pbio_servo_t *mtr, int32_t speed, int32_t duration, pbio_control_after_stop_t after_stop, bool foreground);
pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *mtr, int32_t speed, pbio_control_after_stop_t after_stop);
pbio_error_t pbio_servo_run_angle(pbio_servo_t *mtr, int32_t speed, int32_t angle, pbio_control_after_stop_t after_stop, bool foreground);
pbio_error_t pbio_servo_run_target(pbio_servo_t *mtr, int32_t speed, int32_t target, pbio_control_after_stop_t after_stop, bool foreground);
pbio_error_t pbio_servo_track_target(pbio_servo_t *mtr, int32_t target);
pbio_error_t pbio_servo_set_run_settings(pbio_servo_t *mtr, int32_t max_speed, int32_t acceleration);
pbio_error_t pbio_servo_set_pid_settings(pbio_servo_t *mtr, int16_t pid_kp, int16_t pid_ki, int16_t pid_kd, int32_t tight_loop_time, int32_t position_tolerance, int32_t speed_tolerance, int32_t stall_speed_limit, int32_t stall_time);
pbio_error_t pbio_servo_control_update(pbio_servo_t *mtr);

// TODO: Move these four to control.h once dependency on motor is dropped
void control_init_angle_target(pbio_servo_t *mtr);
void control_init_time_target(pbio_servo_t *mtr);
pbio_error_t control_update_angle_target(pbio_servo_t *mtr, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_control_after_stop_t *actuation_type, int32_t *control);
pbio_error_t control_update_time_target(pbio_servo_t *mtr, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_control_after_stop_t *actuation_type, int32_t *control);



/** @}*/

#endif // _PBIO_SERVO_H_

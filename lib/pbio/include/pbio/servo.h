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
#include <pbio/hbridge.h>
#include <pbio/tacho.h>
#include <pbio/trajectory.h>
#include <pbio/control.h>
#include <pbio/logger.h>

#include <pbio/iodev.h>

#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (128)

#define MAX_ENCMOTOR_SETTINGS_STR_LENGTH (400)

typedef struct _pbio_servo_t {
    pbio_hbridge_t *hbridge;
    pbio_tacho_t *tacho;
    pbio_servo_state_t state;
    pbio_control_t control;
    pbio_port_t port;
    pbio_log_t log;
} pbio_servo_t;

void _pbio_servo_reset_all(void);
void _pbio_servo_poll(void);

pbio_error_t pbio_servo_get(pbio_port_t port, pbio_servo_t **srv, pbio_direction_t direction, fix16_t gear_ratio);  // TODO: Make dc and servo version

pbio_error_t pbio_servo_get_run_settings(pbio_servo_t *srv, int32_t *max_speed, int32_t *acceleration);
pbio_error_t pbio_servo_set_run_settings(pbio_servo_t *srv, int32_t max_speed, int32_t acceleration);

pbio_error_t pbio_servo_get_pid_settings(pbio_servo_t *srv,
                                         int16_t *pid_kp,
                                         int16_t *pid_ki,
                                         int16_t *pid_kd,
                                         int32_t *tight_loop_time,
                                         int32_t *position_tolerance,
                                         int32_t *speed_tolerance,
                                         int32_t *stall_speed_limit,
                                         int32_t *stall_time);
pbio_error_t pbio_servo_set_pid_settings(pbio_servo_t *srv,
                                         int16_t pid_kp,
                                         int16_t pid_ki,
                                         int16_t pid_kd,
                                         int32_t tight_loop_time,
                                         int32_t position_tolerance,
                                         int32_t speed_tolerance,
                                         int32_t stall_speed_limit,
                                         int32_t stall_time);

void pbio_servo_print_settings(pbio_servo_t *srv, char *dc_settings_string, char *enc_settings_string);

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle);
pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled);
pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_control_after_stop_t after_stop); // TODO: Make dc and servo version

pbio_error_t pbio_servo_run(pbio_servo_t *srv, int32_t speed);
pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_control_after_stop_t after_stop, bool foreground);
pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_control_after_stop_t after_stop);
pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_control_after_stop_t after_stop, bool foreground);
pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_control_after_stop_t after_stop, bool foreground);
pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target);


pbio_error_t pbio_logger_start(pbio_log_t *log, int32_t duration);
pbio_error_t pbio_logger_read(pbio_log_t *log, int32_t sindex, uint8_t *len, int32_t *buf);
pbio_error_t pbio_logger_stop(pbio_log_t *log);

pbio_error_t pbio_servo_control_update(pbio_servo_t *srv);

#endif // _PBIO_SERVO_H_

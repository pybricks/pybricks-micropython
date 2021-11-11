// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_SERVO_H_
#define _PBIO_SERVO_H_

#include <stdint.h>

#include <fixmath.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>
#include <pbdrv/counter.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/dcmotor.h>
#include <pbio/tacho.h>
#include <pbio/trajectory.h>
#include <pbio/control.h>
#include <pbio/observer.h>
#include <pbio/logger.h>

#include <pbio/iodev.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

#define PBIO_SERVO_LOG_COLS (9)

typedef struct _pbio_servo_t {
    pbio_port_id_t port;
    bool connected;
    bool claimed;
    pbio_dcmotor_t *dcmotor;
    pbio_tacho_t *tacho;
    pbio_control_t control;
    pbio_observer_t observer;
    pbio_log_t log;
} pbio_servo_t;

pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, fix16_t gear_ratio, bool reset_angle);

pbio_error_t pbio_servo_load_settings(pbio_control_settings_t *control_settings, const pbio_observer_settings_t **observer_settings, pbio_iodev_type_id_t id);

pbio_error_t pbio_servo_get_state(pbio_servo_t *srv, pbio_control_state_t *state);

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs);
pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled);

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop);
void pbio_servo_stop_control(pbio_servo_t *srv);

pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_actuation_t actuation_type, int32_t payload);
pbio_error_t pbio_servo_set_voltage_passive(pbio_servo_t *srv, int32_t voltage);

pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed);
pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop);
pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_actuation_t after_stop);
pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_actuation_t after_stop);
pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop);
pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target);

pbio_error_t pbio_servo_update(pbio_servo_t *srv);

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

#endif // _PBIO_SERVO_H_

// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup Servo Motor: Servo control functions.
 *
 * API for motors with position feedback (servos).
 * @{
 */

#ifndef _PBIO_SERVO_H_
#define _PBIO_SERVO_H_

#include <pbio/config.h>

#if PBIO_CONFIG_SERVO

#include <stdint.h>

#include <fixmath.h>

#include <pbdrv/config.h>
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

#define PBIO_SERVO_LOG_COLS (9)

typedef struct _pbio_servo_t {
    pbio_dcmotor_t *dcmotor;
    pbio_tacho_t *tacho;
    pbio_control_t control;
    pbio_observer_t observer;
    pbio_log_t log;
    pbio_parent_t parent;
    bool run_update_loop;
} pbio_servo_t;

pbio_error_t pbio_servo_get_servo(pbio_port_id_t port, pbio_servo_t **srv);
pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, fix16_t gear_ratio, bool reset_angle);
bool pbio_servo_update_loop_is_running(pbio_servo_t *srv);

pbio_error_t pbio_servo_load_settings(pbio_control_settings_t *control_settings, const pbio_observer_model_t **model, pbio_iodev_type_id_t id);

pbio_error_t pbio_servo_get_state(pbio_servo_t *srv, pbio_control_state_t *state);

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs);
pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop);

pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_actuation_t actuation_type, int32_t payload);
pbio_error_t pbio_servo_set_voltage_passive(pbio_servo_t *srv, int32_t voltage);

pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed);
pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop);
pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_actuation_t after_stop);
pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop);
pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target);

void pbio_servo_update_all(void);

#endif // PBIO_CONFIG_SERVO

#endif // _PBIO_SERVO_H_

/** @} */

#ifndef _PBIO_ENCMOTOR_H_
#define _PBIO_ENCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#include <pbio/dcmotor.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

/**
 * Motor action executed after completing a run command.
 */
typedef enum {
    PBIO_MOTOR_STOP_COAST,      /**< Coast the motor */
    PBIO_MOTOR_STOP_BRAKE,      /**< Brake the motor */
    PBIO_MOTOR_STOP_HOLD,       /**< Actively hold the motor in place */
} pbio_motor_after_stop_t;

/**
 * Busy wait (or not) for a run command to complete
 */
typedef enum {
    PBIO_MOTOR_WAIT_COMPLETION = true, /**< Wait for the run command to complete */
    PBIO_MOTOR_WAIT_NONE = false,       /**< Execute run command in the background and proceed with user program */
} pbio_motor_wait_t;

#define PID_PRESCALE (1000.0)
#define MAX_ENCMOTOR_SETTINGS_STR_LENGTH (200)
#define MS_PER_SECOND (1000.0)

pbio_error_t pbio_encmotor_setup(pbio_port_t port, pbio_id_t device_id, pbio_motor_dir_t direction, float_t gear_ratio);

pbio_error_t pbio_encmotor_set_settings(pbio_port_t port, float_t stall_torque_limit, int16_t max_speed, int16_t tolerance, int16_t acceleration_start, int16_t acceleration_end, int16_t tight_loop_time_ms, int16_t pid_kp, int16_t pid_ki, int16_t pid_kd);

void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string);

pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count);

pbio_error_t pbio_motor_reset_encoder_count(pbio_port_t port, int32_t reset_count);

pbio_error_t pbio_motor_get_angle(pbio_port_t port, float_t *angle);

pbio_error_t pbio_motor_reset_angle(pbio_port_t port, float_t reset_angle);

pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *encoder_rate);

pbio_error_t pbio_motor_get_angular_rate(pbio_port_t port, float_t *angular_rate);

pbio_error_t pbio_motor_run(pbio_port_t port, float_t speed);

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t target);

void motorcontroller();

/** @}*/

#endif // _PBIO_ENCMOTOR_H_

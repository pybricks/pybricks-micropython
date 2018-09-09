
#ifndef _PBIO_MOTOR_H_
#define _PBIO_MOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

typedef float float_t;

/**
 * Motor direction convention.
 */
typedef enum {
    PBIO_MOTOR_DIR_NORMAL,      /**< Use the normal motor-specific convention for the positive direction */
    PBIO_MOTOR_DIR_INVERTED,    /**< Swap positive and negative for both the encoder value and the duty cycle */
} pbio_motor_dir_t;

/**
 * Motor action executed after completing a run command.
 */
typedef enum {
    PBIO_MOTOR_STOP_COAST,      /**< Coast the motor */
    PBIO_MOTOR_STOP_BRAKE,      /**< Brake the motor */
    PBIO_MOTOR_STOP_HOLD,       /**< Actively hold the motor in place */
} pbio_motor_stop_t;

/**
 * Busy wait (or not) for a run command to complete
 */
typedef enum {
    PBIO_MOTOR_WAIT_COMPLETION, /**< Wait for the run command to complete */
    PBIO_MOTOR_WAIT_NONE,       /**< Execute run command in the background and proceed with user program */
} pbio_motor_wait_t;

pbio_error_t pbio_motor_set_constant_settings(pbio_port_t port, int16_t counts_per_unit, float_t gear_ratio);

pbio_error_t pbio_motor_control_set_variable_settings(pbio_port_t port, int16_t max_speed, int16_t tolerance, int16_t acceleration_start, int16_t acceleration_end, int16_t tight_loop_time_ms, int16_t pid_kp, int16_t pid_ki, int16_t pid_kd);

pbio_error_t pbio_motor_run(pbio_port_t port, float_t speed);

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t target);

void motorcontroller();

/** @}*/

#endif // _PBIO_MOTOR_H_

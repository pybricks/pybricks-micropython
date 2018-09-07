
#ifndef _PBIO_MOTORCONTROL_H_
#define _PBIO_MOTORCONTROL_H_

#include <stdint.h>
#include <stdio.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/motor.h>

/**
 * \addtogroup Motorcontrol High-level motor control
 * @{
 */

typedef float float_t;

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

/**
 * Control settings for an encoded motor
 */
typedef struct _motor_control_settings_t {
    int16_t counts_per_output_unit; /**< Encoder counts per output unit, including optional gear train (counts per degree for rotational motors, counts per cm for a linear motor) */
    int16_t max_speed;              /**< Soft limit on the reference speed in all run commands */
    int16_t tolerance;              /**< Allowed deviation (deg) from target before motion is considered complete */
    int16_t acceleration_start;     /**< Acceleration when beginning to move. Positive value in degrees per second per second */
    int16_t acceleration_end;       /**< Deceleration when stopping. Positive value in degrees per second per second */
    int16_t tight_loop_time_ms;     /**< When a run function is called twice in this interval, assume that the user is doing their own speed control.  */
    int16_t pid_kp;                 /**< Proportional position control constant (and integral speed control constant) */
    int16_t pid_ki;                 /**< Integral position control constant */
    int16_t pid_kd;                 /**< Derivative position control constant (and proportional speed control constant) */
} motor_control_settings_t;

motor_control_settings_t motor_control_settings[PBIO_CONFIG_MAX_MOTORS];

pbio_error_t pbio_motor_control_set_constant_settings(pbio_port_t port, int16_t counts_per_unit, float_t gear_ratio);

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

#endif // _PBIO_MOTORCONTROL_H_

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
 * Settings for a DC motor
 */
typedef struct _DCMotor_settings_t {
    pbio_motor_dir_t inverted; /**< Whether or not polarity of duty cycle and encoder counter is inversed. */
    int8_t max_duty;           /**< Soft limit on duty cycle percentage*/
} DCMotor_settings_t;

/**
 * Settings for an Encoded motor
 */
typedef struct _EncodedMotor_settings_t {
    pbio_motor_dir_t inverted;      /**< Whether or not polarity of duty cycle and encoder counter is inversed */
    int8_t max_duty;                /**< Soft limit on duty cycle percentage */
    float_t counts_per_output_unit; /**< Encoder counts per output unit, including optional gear train (counts per degree for rotational motors, counts per cm for a linear motor) */
    float_t max_speed;              /**< Soft limit on the reference speed in all run commands */
    float_t tolerance;              /**< Allowed deviation (deg) from target before motion is considered complete */
    float_t acceleration_start;     /**< Acceleration when beginning to move. Positive value in degrees per second per second */
    float_t acceleration_end;       /**< Deceleration when stopping. Positive value in degrees per second per second */
    uint8_t tight_loop_time_ms;     /**< When a run function is called twice in this interval, assume that the user is doing their own speed control.  */
    float_t pid_kp;                 /**< Proportional position control constant (and integral speed control constant) */
    float_t pid_ki;                 /**< Integral position control constant */
    float_t pid_kd;                 /**< Derivative position control constant (and proportional speed control constant) */
} EncodedMotor_settings_t;

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
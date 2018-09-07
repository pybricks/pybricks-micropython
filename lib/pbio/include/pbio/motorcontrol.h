
#ifndef _PBIO_MOTORCONTROL_H_
#define _PBIO_MOTORCONTROL_H_

#include <stdint.h>
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

pbio_error_t pbio_motor_run(pbio_port_t port, float_t gear_ratio, float_t speed);

pbio_error_t pbio_motor_stop(pbio_port_t port, float_t gear_ratio, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t gear_ratio, float_t speed, float_t duration, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t gear_ratio, float_t speed, float_t *stallpoint, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t gear_ratio, float_t speed, float_t angle, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t gear_ratio, float_t speed, float_t target, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t gear_ratio, float_t target, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

void motorcontroller();

/** @}*/

#endif // _PBIO_MOTORCONTROL_H_
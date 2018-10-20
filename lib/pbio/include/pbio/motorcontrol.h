#ifndef _PBIO_MOTORCONTROL_H_
#define _PBIO_MOTORCONTROL_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>

/**
 * \addtogroup Motor Motors
 * @{
 */


/**
 * Motor controlled stop type for use with pbio_encmotor_stop
 */
typedef enum {
    PBIO_MOTOR_STOP_FAST,        /**< Immediate stop at the current location */
    PBIO_MOTOR_STOP_SMOOTH,      /**< Controlled deceleration equivalent to the final part of a run maneuver */
} pbio_motor_controlled_stop_t;

/**
 * Motor action executed after completing a run command that ends in a smooth stop.
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
    PBIO_MOTOR_WAIT_WAIT = true, /**< Wait for the run command to complete */
    PBIO_MOTOR_WAIT_BACKGROUND = false,       /**< Execute run command in the background and proceed with user program */
} pbio_motor_wait_t;

pbio_error_t pbio_encmotor_run(pbio_port_t port, int32_t speed);

pbio_error_t pbio_encmotor_stop(pbio_port_t port, pbio_motor_controlled_stop_t smooth, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_time(pbio_port_t port, int32_t speed, int32_t duration, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_stalled(pbio_port_t port, int32_t speed, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_angle(pbio_port_t port, int32_t speed, int32_t angle, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_target(pbio_port_t port, int32_t speed, int32_t target, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_track_target(pbio_port_t port, int32_t target);

#ifdef PBIO_CONFIG_ENABLE_MOTORS
void _pbio_motorcontrol_poll(void);
#else
static inline void _pbio_motorcontrol_poll(void) { }
#endif // PBIO_CONFIG_ENABLE_MOTORS

/** @}*/

#endif // _PBIO_MOTORCONTROL_H_

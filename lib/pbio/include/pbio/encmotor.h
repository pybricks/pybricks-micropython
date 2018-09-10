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
} pbio_motor_stop_t;

/**
 * Busy wait (or not) for a run command to complete
 */
typedef enum {
    PBIO_MOTOR_WAIT_COMPLETION, /**< Wait for the run command to complete */
    PBIO_MOTOR_WAIT_NONE,       /**< Execute run command in the background and proceed with user program */
} pbio_motor_wait_t;

#define PID_PRESCALE (1000.0)
#define MAX_ENCMOTOR_SETTINGS_STR_LENGTH (200)
#define MS_PER_SECOND (1000.0)

pbio_error_t pbio_encmotor_set_constant_settings(pbio_port_t port, int16_t counts_per_unit, float_t gear_ratio);

pbio_error_t pbio_encmotor_set_variable_settings(pbio_port_t port, int16_t max_speed, int16_t tolerance, int16_t acceleration_start, int16_t acceleration_end, int16_t tight_loop_time_ms, int16_t pid_kp, int16_t pid_ki, int16_t pid_kd);

pbio_error_t pbio_encmotor_print_settings(pbio_port_t port, char *settings_string);

/**
 * Gets the tachometer encoder count.
 * @param [in]  port    The motor port
 * @param [out] count   The count
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count);
pbio_error_t pbio_motor_reset_encoder_count(pbio_port_t port, int32_t reset_count);

pbio_error_t pbio_motor_get_angle(pbio_port_t port, float_t *angle);
pbio_error_t pbio_motor_reset_angle(pbio_port_t port, float_t reset_angle);

/**
 * Gets the tachometer encoder rate in counts per second.
 * @param [in]  port    The motor port
 * @param [out] rate    The rate
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *encoder_rate);
pbio_error_t pbio_motor_get_angular_rate(pbio_port_t port, float_t *angular_rate);

pbio_error_t pbio_motor_run(pbio_port_t port, float_t speed);

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_stop_t stop, pbio_motor_wait_t wait);

pbio_error_t pbio_motor_track_target(pbio_port_t port, float_t target);

void motorcontroller();

/** @}*/

#endif // _PBIO_ENCMOTOR_H_
